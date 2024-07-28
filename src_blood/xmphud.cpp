/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Xmapedit HUD implementation for both 2d and 3d modes.
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
#include "xmpmaped.h"
#include "xmparted.h"
#include "xmphud.h"
#include "tile.h"

#define kMsgScrollStartDelay		128
#define kMsgScrollDelay				10


char* gHudLayoutNames[] =
{
	"Disabled",			// 0
	"Always full",		// 1
	"Always compact",	// 2
	"Always slim",		// 3
	"Dynamic",			// 4
};

char* gHudFontNames[kHudFontPackMax] =
{
	"Tiny",				// 0
	"Smaller",			// 1
	"Small",			// 2
	"Normal",			// 3
	"Large",			// 4
	"Larger",			// 5
	"Huge",				// 6
};

static unsigned char gFontPack[kHudFontPackMax][kHudFontMax][2] =
{
	{	// tiny
		{2,		2},		// message
		{2,		1},		// content
		{2,		2},		// status
		{3,		2},		// logo
	},
	
	{
		// smaller
		{0,		8},
		{0,		1},
		{0,		8},
		{3,		2},
	},
	
	{
		// small
		{0,		8},
		{9,		2},
		{0,		8},
		{3,		2},
	},
	
	{
		// normal
		{9,		9},
		{10,	2},
		{9,		9},
		{3,		2},
	},
	
	{
		// large
		{9,		9},
		{11,	8},
		{9,		9},
		{3,		2},
	},
	
	{
		// larger
		{10,	10},
		{12,	8},
		{10,	10},
		{3,		2},
	},
	
	{
		// huge
		{10,	10},
		{13,	8},
		{10,	10},
		{3,		2},
	},
};


MAPEDIT_HUD gMapedHud;

void MAPEDIT_HUD::SetFontPack(int nFontPack)
{
	int i;
	if (nFontPack >= 0)
	{
		SetView(layout, nFontPack);
		UpdateView();
	}
}

void MAPEDIT_HUD::SetView(int x1, int y1, int x2, int y2)
{	
	ROMFONT *fmsg	= GetFont(kHudFontMsg);
	ROMFONT *fcont	= GetFont(kHudFontCont);
	ROMFONT *fstat	= GetFont(kHudFontStat);
	
	int ts, t, wh = x2-x1, hg = y2-y1;
	int pd = fcont->lh>>1;
	tw = fmsg->lh<<2;
	t = tw>>1;
	
	lgw = ClipLow(fmsg->lh * 22, 180);
	
	main.Setup(x1, y1, x2, y2);
	
	wide		= (main.wh >= 1000);
	wide800		= (main.wh >= 768);
	slim		= (layout == kHudLayoutSlim);
	draw		= (main.wh >= 320 && main.hg && layout != kHudLayoutNone);
	
	if (!draw)
	{
		layout = kHudLayoutNone;
		editScrBox.Setup(0, 0, 0, 0);
		message.Setup(0, 0, 0, 0);
		tilebox.Setup(0, 0, 0, 0);
		content.Setup(0, 0, 0, 0);
		zoom2d.Setup(0, 0, 0, 0);
		coords.Setup(0, 0, 0, 0);
		grid2d.Setup(0, 0, 0, 0);
		bottom.Setup(0, 0, 0, 0);
		logo.Setup(0, 0, 0, 0);
		main.Setup(0, 0, 0, 0);
		tileInfo.draw = editScr.draw = 0;
		lgw = tw = 0;
		UpdateMask();
		return;
	}
	
	logo.Setup(x1, y1, x1+lgw, y1+tw);
	message.Setup(x1+lgw-t, y1+t, x2, y1+tw);
	
	t = fstat->lh<<1;
	content.Setup(x1+pd, y1+tw+pd, ClipHigh(x1+(81*fcont->ls), main.wh), y2-t-pd);

	ts = hg-tw-t;
	tilebox.Setup(x2-ts+pd, y1+tw+pd, x2-pd, y2-t-pd);
	tileInfo.inside = tilebox.Inside(&main);

	if (!tileInfo.inside)
	{
		ts = mulscale16(gHudPrefs.tileScaleSize, ClipLow(0x1300*fcont->lh, 0xD8F0));
		tilebox.Setup(x2-ts-t-pd, y1-ts-(t>>1), x2-pd, y1+t-(t>>1));
		tileInfo.draw = 1;
	}
	else
	{
		tileInfo.draw = (tilebox.Inside(&content, 1) == 0);
	}

	if (wide)
	{
		zoom2d.Setup(x1,		y2-t, x1+perc2val(10, wh), y2);
		coords.Setup(zoom2d.x2, y2-t, zoom2d.x2+perc2val(30, wh), y2);
		grid2d.Setup(coords.x2, y2-t, coords.x2+perc2val(10, wh), y2);
	}
	else
	{
		coords.Setup(x1, y2-t, x1+perc2val(38, wh), y2);
	}
	
	status.Setup((wide) ? grid2d.x2 : coords.x2, y2-t, x2, y2);
	
	editScr.draw = (layout == kHudLayoutFull);
	
	if (editScr.draw)
	{
		int tx2 = tilebox.x1 - pd;
		int tx1 = content.x2 + pd;
		int twh = tx2-tx1;
		editScr.draw = (twh >= 150);
		if (editScr.draw)
		{
			editScrBox.Setup(tx2-ClipHigh(twh, 400), y1+tw+pd, tx2, y2-t-pd);
			
			SCREEN2D* pScr = &editScr.screen2D;
			memcpy(pScr->colors, gScreen2D.colors, sizeof(gScreen2D.colors));
			memcpy(&pScr->prefs, &gScreen2D.prefs, sizeof(gScreen2D.prefs));
			
			pScr->data.grid				= 0;
			pScr->data.zoom				= 384;
			pScr->prefs.useTransluc		= 0;
			pScr->prefs.showTags		= 0;
			pScr->prefs.showMap			= 0;
			pScr->prefs.ambRadius		= 3;
			pScr->prefs.ambRadiusHover	= 0;
			pScr->prefs.showVertex		= 0;
			pScr->prefs.showHighlight	= 0;
			pScr->prefs.showFeatures	= 0;
			
			pScr->SetView
			(
				editScrBox.x1+pd, editScrBox.y1+pd,
				editScrBox.x2-pd, editScrBox.y2-pd
			);
		}
	}
	
	int tx1 = content.x1, tx2 = content.x2;
	int tx3, cwh;
	
	if (editScr.draw)							tx3 = editScrBox.x1;
	else if (tileInfo.inside && tileInfo.draw)	tx3 = tilebox.x1;
	else										tx3 = x2;
	
	cwh = tx3-tx2;
	if (cwh >= 320)
	{
		cwh>>=1;
		content.Setup(x1+cwh+pd, y1+tw+pd, x1+cwh+pd+(80*fcont->ls), y2-t-pd);
	}
	
	bottom.Setup(x1, y2-t, x2, y2);
	UpdateMask();
}

void MAPEDIT_HUD::SetView(int nLayout, int nFontPack)
{
	int x1, y1, x2, y2, hg = 0;
	int i, nID1, nID2;
	if (nFontPack >= 0)
	{
		for (i = 0; i < kHudFontMax; i++)
		{
			int nID1 = gFontPack[nFontPack][i][0];
			int nID2 = gFontPack[nFontPack][i][1];
			
			fonts[i][0]	= &vFonts[nID1];
			fonts[i][1]	= &vFonts[nID2];
		}
		
		fontPack = nFontPack;
	}
	
	layout = nLayout;
	
	switch(nLayout)
	{
		case kHudLayoutNone:
			break;
		case kHudLayoutCompact:
			hg += (GetFont(kHudFontMsg)->lh		<< 2);
			hg += (GetFont(kHudFontStat)->lh	<< 1);
			break;
		case kHudLayoutSlim:
			hg += (GetFont(kHudFontMsg)->lh		<< 2);
			hg +=1;
			break;
		default:
			layout = kHudLayoutFull;
			hg += (GetFont(kHudFontMsg)->lh		<< 2);
			hg += (GetFont(kHudFontStat)->lh	<< 1);
			hg += (GetFont(kHudFontCont)->lh	* 11);
			hg += (GetFont(kHudFontCont)->lh	>> 1);
			break;
	}
	
	
	
	x1 = 0;
	x2 = xdim;
	y1 = ydim - hg;
	y2 = ydim;
	
	SetView(x1, y1, x2, y2);
	
}

void MAPEDIT_HUD::UpdateView()
{
	int tmp;
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs;
	SetLogo(prefs->header, prefs->text, prefs->icon);
	
	MAPEDIT_HUD_TILE* tile =& tileInfo;
	tmp = tileInfo.nTile; tile->nTile = (tile->nTile == -1) ? 0 : -1; // to force update
	SetTile(tmp, tile->nPlu, tile->nShade);
	
	UpdateMask();
	
	if (draw)
		DrawIt();
}

void MAPEDIT_HUD::SetLogo(char* header, char* text, int nTile, int nPlu, int nShade)
{
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs;
	ROMFONT* pFontF = GetFont(kHudFontLogo, 0);
	ROMFONT* pFontS = GetFont(kHudFontLogo, 1);
	int x1, x2, y1, y2, x3, y3, iw, ih;
	int t, l1, l2;
	
	if (header)
		strcpy(prefs->header, header);
	
	if (text)
		strcpy(prefs->text, text);
	
	prefs->icon 		= nTile;
	prefs->iconScale 	= perc2val(90, tw);
	prefs->iconShade	= nShade;
	prefs->iconPlu		= nPlu;
	prefs->iconx 		= 0;
	prefs->icony 		= 0;
	prefs->headx		= 0;
	prefs->heady		= 0;
	prefs->textx 		= 0;
	prefs->texty 		= 0;
	
	logo.GetCoords(&x1, &y1, &x2, &y2);
	l1 = strlen(prefs->header)*pFontF->ls;
	l2 = strlen(prefs->text)*pFontS->ls;
	if (l1 < l2)
		l1 = l2;
	
	t = pFontF->lh;
	if (l2)
		t += pFontS->lh;
	
	x3 = x1+(((lgw-tw)>>1)-(l1>>1));
	if (prefs->icon >= 0)
	{
		tileDrawGetSize(prefs->icon, prefs->iconScale, &iw, &ih);
		prefs->headx = x3+4+(iw>>1);
		prefs->heady = y1+(tw>>1)-(t>>1);	
		prefs->iconx = ClipLow(x3-4-(iw>>1), x1);
		prefs->icony = ClipLow(y1+(tw>>1)-(ih>>1), y1);
	}
	else
	{
		prefs->headx = x3;
		prefs->heady = y1+(tw>>1)-(t>>1);
	}
	
	prefs->textx = prefs->headx+((l1>>1)-(l2>>1));
	prefs->texty = prefs->heady+pFontF->lh+1;
}

void MAPEDIT_HUD::DrawStatus(MAPEDIT_HUD_WINDOW* w, short lCol, short bCol)
{
	if (bCol >= 0)
	{
		gfxSetColor(bCol);
		gfxFillBox(w->x1, w->y1, w->x2, w->y2);
	}
	
	if (lCol >= 0)
	{
		gfxSetColor(lCol);
		gfxRect(w->x1, w->y1, w->x2, w->y2);
	}
}

void MAPEDIT_HUD::DrawMask()
{
	int i, x1, x2, y1, y2, t = tw>>1;
	int x, y, tis, r;
	
	main.GetCoords(&x1, &y1, &x2, &y2);
	gfxSetColor(clr2std(kColorDarkGray));
	gfxFillBox(x1, y1+tw, x2, y2);
	
	gfxSetColor(clr2std(kColorGrey25)); gfxVLine(x1, y1, y2);
	gfxSetColor(clr2std(kColorGrey28)); gfxVLine(x2, y1+t, y2);
	gfxHLine(y2, x1, x2);
	
	logo.GetCoords(&x1, &y1, &x2, &y2);
	gfxSetColor(clr2std(kColorGrey25)); gfxHLine(y1, x1, x2 - tw);
	gfxVLine(x1, y1, y2);
	
	gfxSetColor(clr2std(kColorGrey28));
	for (i = 0; i < tw; i++)
		gfxPixel(x2 + i - tw, y1 + i);
	
	message.GetCoords(&x1, &y1, &x2, &y2);
	gfxSetColor(clr2std(kColorGrey25)); gfxHLine(y1-1, x1, x2);
	gfxSetColor(clr2std(kColorGrey28)); gfxHLine(y2, x1 + t, x2);
	
	if (tileInfo.draw && tileInfo.inside)
	{
		tilebox.GetCoords(&x1, &y1, &x2, &y2);
		tis   = tileInfo.size;
		r 	  = tileInfo.marg;
		
		x = x1+r; y = y1+r;
		gfxSetColor(clr2std(kColorBrown));
		gfxFillBox(x, y, x+tis, y+tis);
		gfxRect(x1, y1, x2, y2);

		gfxSetColor(clr2std(kColorGrey28));
		gfxVLine(x+(tis>>1), y+r, y+tis-r);
		gfxHLine(y+(tis>>1), x+r, x+tis-r);
	}
	
	if (editScr.draw)
	{
		editScrBox.GetCoords(&x1, &y1, &x2, &y2);
		gfxSetColor(clr2std(kColorBrown));
		gfxRect(x1, y1, x2, y2);
		
		SCREEN2D* pScr = &editScr.screen2D;
		gfxSetColor(clr2std(kColorGrey29));
		gfxFillBox(pScr->view.wx1, pScr->view.wy1, pScr->view.wx2, pScr->view.wy2);
	}
	
	DrawStatus(&coords, clr2std(kColorGrey25), clr2std(kColorGrey28));
	DrawStatus(&status, clr2std(kColorGrey25), clr2std(kColorGrey27));
	DrawStatus(&zoom2d, clr2std(kColorGrey25), clr2std(kColorGrey29));
	DrawStatus(&grid2d, clr2std(kColorGrey25), clr2std(kColorGrey27));
}

void MAPEDIT_HUD::DrawLogo()
{
	int x1, x2, y1, y2, x3, i;
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs;
	ROMFONT* pFontF = GetFont(kHudFontLogo, 0);
	ROMFONT* pFontS = GetFont(kHudFontLogo, 1);
	logo.GetCoords(&x1, &y1, &x2, &y2);

	gfxSetColor(clr2std(kColorDarkGray));
	for (i = 0, x3 = x2 - tw - 1; i < tw; i++)
		gfxHLine(y1 + i, x1, x3 + i);
	
	gfxSetColor(clr2std(kColorGrey25));
	gfxHLine(y1, x1, x3);
	gfxVLine(x1, y1, y2);

	printextShadow(prefs->headx, prefs->heady, clr2std(kColorYellow), prefs->header, pFontF);
	printextShadow(prefs->textx, prefs->texty, clr2std(kColorLightMagenta), prefs->text, pFontS);
	
	if (prefs->icon >=0)
		tileDrawTile(prefs->iconx, prefs->icony, tileGetPic(prefs->icon), prefs->iconScale, prefs->iconPlu, 0x02, prefs->iconShade);
}

void MAPEDIT_HUD::DrawStatus()
{
	PrintCoords();
	PrintStats();
	
	if (wide)
	{
		PrintZoom2d();
		PrintGrid2d();
	}
}

void MAPEDIT_HUD::UpdateMask()
{
	int nTile = gSysTiles.hudMask;
	int hg = HeightNoLogo();
	int wh = Width();
	int th = tw>>1;
	
	if (tileLoadTile(nTile))
		tileFreeTile(nTile);
	
	if (wh && hg)
	{
		// prepare mask tile *without* logo part,
		// which makes it possible to draw without
		// transparency flag for better performance
		
		if (tileAllocTile(nTile, wh, hg))
		{
			setviewtotile(gSysTiles.drawBuf, ydim, xdim);
			DrawMask();
			ClearMessageArea();
			setviewback();
			
			artedRotateTile(gSysTiles.drawBuf);
			artedFlipTileY(gSysTiles.drawBuf);
			copytilepiece(gSysTiles.drawBuf, main.x1, main.y2-hg, wh, hg, nTile, 0, 0);
		}
	}
}

void MAPEDIT_HUD::DrawIt()
{
	if (tileLoadTile(gSysTiles.hudMask))
	{
		tileDrawTile(main.x1, main.y2-HeightNoLogo(), gSysTiles.hudMask, 0, 0, 0, 0);
	}
	else
	{
		DrawMask();
		ClearMessageArea();
	}

	DrawLogo();
	PrintMessage();
	
	if (!slim)
		DrawStatus();
	
	if (tileInfo.draw)
		DrawTile();
	
	if (editScr.draw)
		DrawEditScreen();
}

void MAPEDIT_HUD::ClearContent()
{
	int x1, x2, y1, y2;
	content.GetCoords(&x1, &y1, &x2, &y2);
	gfxSetColor(clr2std(kColorDarkGray));
	gfxFillBox(x1, y1, x2, y2);
}

void MAPEDIT_HUD::ClearMessageArea()
{
	int x1, x2, y1, y2, x3, i, t = tw >> 1;
	message.GetCoords(&x1, &y1, &x2, &y2);
	
	gfxSetColor(clr2std(kColorBrown));
	for (i = 0, x3 = x1+1; i < t; i++)
		gfxHLine(y1+i, x3+i, x2);
}

void MAPEDIT_HUD::SetMsgImp(int nTime, char *__format, ...)
{
	MAPEDIT_HUD_MSG* pData =& msgData;
	//if (pData->showTicks >= 0 && totalclock >= pData->showTicks)
	{
		char tmp[256];
		va_list argptr;
		va_start(argptr, __format);
		pData->textLen = vsprintf(tmp, __format, argptr);
		va_end(argptr);
		
		if (stricmp(tmp, pData->message) != 0)
		{
			sprintf(pData->message, "%s", tmp);
			pData->showTicks	= totalclock + nTime;
			pData->scrollTicks	= totalclock + kMsgScrollStartDelay;
			pData->scrollDir	= 0;
			pData->textPos		= 0;
		}
	}
}

void MAPEDIT_HUD::SetMsg(char *__format, ...)
{
	MAPEDIT_HUD_MSG* pData =& msgData;
	if (pData->showTicks >= 0 && totalclock >= pData->showTicks)
	{
		char tmp[256];
		
		pData->showTicks = 0;

		va_list argptr;
		va_start(argptr, __format);
		pData->textLen = vsprintf(tmp, __format, argptr);
		va_end(argptr);
		
		if (stricmp(tmp, pData->message) != 0)
		{
			sprintf(pData->message, "%s", tmp);
			pData->scrollTicks	= totalclock + kMsgScrollStartDelay;
			pData->scrollDir	= 0;
			pData->textPos		= 0;
		}
	}
}

void MAPEDIT_HUD::SetMsg()
{
	MAPEDIT_HUD_MSG* pData =& msgData;
	if (pData->showTicks >= 0 && totalclock >= pData->showTicks)
	{
		pData->showTicks = 0;
		pData->message[0] = '\0';
	}
}

void MAPEDIT_HUD::PrintMessage()
{
	char tcol;
	int x1, x2, y1, y2, t = tw >> 1;
	ROMFONT* pFont = GetFont(kHudFontMsg);
	MAPEDIT_HUD_MSG* pData =& msgData;
	
	tcol = clr2std((pData->showTicks && h) ? kColorYellow : kColorGrey17);
	message.GetCoords(&x1, &y1, &x2, &y2);
	//ClearMessageArea();
	
	if (pData->scrollTicks && totalclock >= pData->scrollTicks)
	{
		if (x1+(pData->textLen*pFont->ls) > x2)
		{
			if (!pData->scrollDir)
			{
				int x3 = x1+((pData->textLen - pData->textPos)*pFont->ls);
				if (x3 >= x1+perc2val(75, x2-x1))
				{
					pData->textPos = ClipHigh(pData->textPos + 1, pData->textLen - 1);
					pData->scrollTicks = totalclock + kMsgScrollDelay;
				}
				else
				{
					pData->scrollTicks = totalclock + (kMsgScrollStartDelay>>1);
					pData->scrollDir = 1;
				}
			}
			else if (pData->textPos)
			{
				pData->textPos = ClipLow(pData->textPos - 1, 0);
				pData->scrollTicks = totalclock + kMsgScrollDelay;
			}
			else
			{
				pData->scrollTicks = totalclock + (kMsgScrollStartDelay>>2);
				pData->scrollDir = 0;
			}
		}
		else
		{
			pData->scrollTicks = 0;
		}
	}
	
	printextShadow(x1 + t, message.my-(pFont->lh>>1), tcol, &pData->message[pData->textPos], pFont);
}

void MAPEDIT_HUD::PrintCoords()
{
	char tmp[64];
	int x1, x2, y1, y2, len;
	coords.GetCoords(&x1, &y1, &x2, &y2);
	ROMFONT* pFont = GetFont(kHudFontStat);
	
	if (wide)
		len = sprintf(tmp, "x:%-6d y:%-6d z:%-6d ang:%-4d", posx, posy, posz, ang);
	else
		len = sprintf(tmp, "x:%-6d y:%-6d ang:%-4d", posx, posy, ang);
	
	CenterText(&x1, &y1, coords.wh, len, coords.hg, pFont);
	printextShadow(x1, y1, clr2std(kColorGrey20), strupr(tmp), pFont);
}


void MAPEDIT_HUD::PrintZoom2d()
{
	char tmp[64];
	int x1, x2, y1, y2, len;
	zoom2d.GetCoords(&x1, &y1, &x2, &y2);
	ROMFONT* pFont = GetFont(kHudFontStat);
	
	if (qsetmode != 200)
		len = sprintf(tmp, "zoom:%d", zoom);
	else
		len = sprintf(tmp, "%s", gZModeNames[zmode]);
	
	CenterText(&x1, &y1, zoom2d.wh, len, zoom2d.hg, pFont);
	printextShadow(x1, y1, clr2std(kColorLightBlue), strupr(tmp), pFont);
}

void MAPEDIT_HUD::PrintGrid2d()
{
	char tmp[64];
	int nGrid = grid, x1, x2, y1, y2, len;
	grid2d.GetCoords(&x1, &y1, &x2, &y2);
	ROMFONT* pFont = GetFont(kHudFontStat);
	
	if (qsetmode == 200 && gMousePrefs.fixedGrid)
		nGrid = gMousePrefs.fixedGrid;
	
	len = sprintf(tmp, "grid:%d", nGrid);
	CenterText(&x1, &y1, grid2d.wh, len, grid2d.hg, pFont);
	printextShadow(x1, y1, clr2std(kColorLightGreen), strupr(tmp), pFont);
}



void MAPEDIT_HUD::PrintStats()
{	
	char tmp[128];
	int x1, x2, y1, y2, len;
	status.GetCoords(&x1, &y1, &x2, &y2);
	BOOL xstats = (qsetmode != 200 && keystatus[KEY_CAPSLOCK]);
	ROMFONT* pFont = GetFont(kHudFontStat);
		
	if (wide)
	{
		if (xstats)
		{
			len = sprintf(tmp, "xsectors: %d/%d # xwalls: %d/%d # xsprites: %d/%d",
				numxsectors, kMaxXSectors, numxwalls, kMaxXWalls, numxsprites, kMaxXSprites);
		}
		else
		{
			len = sprintf(tmp, "sectors: %d/%d # walls: %d/%d # sprites: %d/%d",
				numsectors, kMaxSectors, numwalls, kMaxWalls, numsprites, kMaxSprites);
		}
	}
	else if (xstats)
	{
		len = sprintf(tmp, "xsec:%d/%d # xwal:%d/%d # xspr:%d/%d",
				numxsectors, kMaxXSectors, numxwalls, kMaxXWalls, numxsprites, kMaxXSprites);
	}
	else
	{
		len = sprintf(tmp, "sec:%d/%d # wal:%d/%d # spr:%d/%d",
				 numsectors, kMaxSectors, numwalls, kMaxWalls, numsprites, kMaxSprites);
	}
	
	CenterText(&x1, &y1, status.wh, len, status.hg, pFont);
	printextShadow(x1, y1, clr2std(kColorLightMagenta), strupr(tmp), pFont);
}

void MAPEDIT_HUD::DrawEditScreen()
{
	if (gMapLoaded)
	{
		SCREEN2D* pScr = &editScr.screen2D;
		
		if (qsetmode == 200)
		{
			pScr->data.camx	= posx;
			pScr->data.camy	= posy;
			pScr->data.ang	= ang;
			
			gfxBackupClip();
			gfxSetClip(pScr->view.wx1, pScr->view.wy1, pScr->view.wx2, pScr->view.wy2);
			
			pScr->ScreenClear();
			pScr->ScreenDraw();
			
			gfxRestoreClip();
		}
		else if (cursectnum >= 0)
		{
			int bwx1 = windowx1;
			int bwx2 = windowx2;
			int bwy1 = windowy1;
			int bwy2 = windowy2;
			
			
			setview(pScr->view.wx1, pScr->view.wy1, pScr->view.wx2, pScr->view.wy2);
			processDrawRooms();
			setview(bwx1, bwy1, bwx2, bwy2);
		}
	}
}

void MAPEDIT_HUD::DrawTile()
{
	char buf[32];
	int nTile, nPlu, nShade, tis, r;
	int x, y, x1, x2, y1, y2, ln;
	tilebox.GetCoords(&x1, &y1, &x2, &y2);
	ROMFONT* pFontL = GetFont(kHudFontCont, 0);
	ROMFONT* pFontS = GetFont(kHudFontCont, 1);
	
	nTile = tileInfo.nTile; nPlu = tileInfo.nPlu; nShade = tileInfo.nShade;
	tis   = tileInfo.size;
	r 	  = tileInfo.marg;
	
	if (!tileInfo.inside)
	{
		gfxSetColor(clr2std(kColorDarkGray));
		gfxFillBox(x1, y1, x2, y2);
	
		x = x1+r; y = y1+r;
		gfxSetColor(clr2std(kColorBrown));
		gfxFillBox(x, y, x+tis, y+tis);
		gfxRect(x1, y1, x2, y2);

		gfxSetColor(clr2std(kColorGrey28));
		gfxVLine(x+(tis>>1), y+r, y+tis-r);
		gfxHLine(y+(tis>>1), x+r, x+tis-r);
	}
	
	if (nTile >= 0)
	{
		x = tileInfo.dx;
		y = tileInfo.dy;
		
		if (tilesizx[nTile])
		{
			tileDrawTile(x, y, nTile, tileInfo.size, nPlu, 0x02, nShade);
		}
		else
		{
			printextShadow(x, y, clr2std(kColorWhite), "<BLANK>", pFontL);
		}
		
		x = x1+r; y = y1+r;
		sprintf(buf, "%d", nTile);
		printextShadow(x, y, clr2std(kColorYellow), buf, pFontL);
		
		if (nPlu)
		{
			sprintf(buf, "%d", nPlu);
			printextShadow(x, y+pFontL->lh, clr2std(kColorGrey18), buf, pFontL);
		}
		
		if (nShade)
		{
			ln = sprintf(buf, "%d", nShade)*pFontL->ls;
			printextShadow(x+tis-ln, y, clr2std(kColorGrey18), buf, pFontL);
		}
		
		x = x1+r; y = y1+tis;
		if (pFontS == (ROMFONT*)&vFonts[1])
			y-=4;
		
		ln = sprintf(buf, surfNames[surfType[nTile]])*pFontS->ls;
		printextShadow(x, y, clr2std(kColorGrey18), strupr(buf), pFontS);
		
		ln = sprintf(buf, "%dx%d", tilesizx[nTile], tilesizy[nTile])*pFontS->ls; x += (tis-ln);
		printextShadow(x, y, clr2std(kColorGrey18), strupr(buf), pFontS);
	}
}

void MAPEDIT_HUD::SetTile(int nTile, int nPlu, int nShade)
{
	int nTile2, tis, tiw, tih, ln;
	ROMFONT* pFontL = GetFont(kHudFontCont, 0);
	ROMFONT* pFontS = GetFont(kHudFontCont, 1);
	nTile2 = tileInfo.nTile;
	
	tileInfo.nPlu	= nPlu;
	tileInfo.nShade = (gHudPrefs.tileShowShade) ? nShade : 0;
	tileInfo.nTile 	= nTile;
	
	if (nTile >= 0)
	{
		if (nTile2 >= 0)
		{
			if (tilesizx[nTile2] == tilesizx[nTile]
					&& tilesizy[nTile2] == tilesizy[nTile]) return;
		}
	}
	
	tis = tilebox.hg-pFontL->lh;
	tileInfo.marg  = pFontL->lh>>1;
	tileInfo.size  = tis;

	if (nTile >= 0)
	{
		if (tilesizx[nTile])
		{
			tileDrawGetSize(nTile, tis, &tiw, &tih);
			tileInfo.dx = tilebox.mx-(tiw>>1);
			tileInfo.dy = tilebox.my-(tih>>1);
		}
		else
		{
			ln = strlen("<BLANK>")*pFontL->ls;
			tileInfo.dx = tilebox.mx-(ln>>1);
			tileInfo.dy = tilebox.my-(pFontL->lh>>1);
		}
	}
}

void MAPEDIT_HUD::CenterText(int* x, int* y, int wh, int len, int hg, ROMFONT* pFont)
{
	*x += ((wh>>1)-((len*pFont->ls)>>1));
	*y += ((hg>>1)-(pFont->lh>>1))+1;
}

void hudSetFontPack(MAPEDIT_HUD* pHud, int fontPack, MOUSE* pMouse)
{
	pHud->SetFontPack(fontPack);
	
	if (pMouse)
		pMouse->RangeSet(0, 0, xdim, ydim-pHud->Height());
}

void hudSetLayout(MAPEDIT_HUD* pHud, int layoutType, MOUSE* pMouse)
{
	pHud->SetView(layoutType, -1);
	pHud->UpdateView();
	
	if (pMouse)
	{
		gMouseLook.maxSlopeF = ClipHigh(gMouseLook.maxSlope, (widescreen) ? 100 : 200);
		pMouse->Init(&gMousePrefs);
		
		pMouse->RangeSet(0, 0, xdim, ydim-pHud->Height());
	}	
}
