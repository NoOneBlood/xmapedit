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
#include "editor.h"
#include "gui.h"
#include "db.h"
#include "tile.h"
#include "keyboard.h"
#include "xmpconf.h"
#include "xmpstub.h"
#include "xmphud.h"
#include "xmpmisc.h"
#include "xmparted.h"

char* gHudLayoutNames[] = {
	
	"Disabled",			// 0
	"Always full",		// 1
	"Always compact",	// 2
	"Always slim",		// 3
	"Dynamic",			// 4
	
};

MAPEDIT_HUD gMapedHud;
void MAPEDIT_HUD::InitColors() {
	
	this->colors.mainbg		= clr2std(kColorDarkGray);
	this->colors.logobg		= this->colors.mainbg;
	this->colors.logofr		= clr2std(kColorYellow);
	this->colors.cordbg		= clr2std(28);
	this->colors.cordfr		= clr2std(20);
	this->colors.mesgbg		= clr2std(kColorBrown);
	this->colors.mesgfr1	= clr2std(18);
	this->colors.mesgfr2	= clr2std(kColorYellow);
	this->colors.statbg		= clr2std(27);
	this->colors.statfr		= clr2std(kColorLightMagenta);
	this->colors.hintbg		= clr2std(27);
	this->colors.hintfr		= clr2std(18);
	this->colors.line1		= clr2std(25);
	this->colors.line2		= clr2std(28);
	this->colors.gridbg		= clr2std(27);
	this->colors.gridfr		= clr2std(kColorLightGreen);
	this->colors.zoombg		= clr2std(29);
	this->colors.zoomfr		= clr2std(kColorLightBlue);
	this->colors.tilnum		= clr2std(kColorYellow);
	this->colors.tilplu		= clr2std(18);
	this->colors.tilsiz		= clr2std(18);
	this->colors.tilsrf		= clr2std(18);
	this->colors.tilblk		= clr2std(kColorWhite);
	this->colors.tilbg		= clr2std(kColorBrown);
	this->colors.tilshd		= clr2std(17);
	this->colors.cmtbg		= this->colors.mainbg;
	this->colors.cmtfr		= clr2std(kColorWhite);
	
	
}

void MAPEDIT_HUD::GetWindowCoords(MAPEDIT_HUD_WINDOW* from, int *ox1, int *oy1, int *ox2, int *oy2)
{
	*ox1 = from->x1;
	*oy1 = from->y1;
	*ox2 = from->x2;
	*oy2 = from->y2;
}

void MAPEDIT_HUD::GetWindowSize(MAPEDIT_HUD_WINDOW* from, int *ow, int *oh)
{
	if (ow) *ow = from->x2 - from->x1;
	if (oh) *oh = from->y2 - from->y1;
}

BOOL MAPEDIT_HUD::WindowInside(MAPEDIT_HUD_WINDOW* w1, MAPEDIT_HUD_WINDOW* w2)
{
	if (!irngok(w1->x1, w2->x1, w2->x2)) return FALSE;
	if (!irngok(w1->y1, w2->y1, w2->y2)) return FALSE;
	if (!irngok(w1->x2, w2->x1, w2->x2)) return FALSE;
	if (!irngok(w1->y2, w2->y1, w2->y2)) return FALSE;
	return TRUE;
}

void MAPEDIT_HUD::SetView(int x1, int y1, int x2, int y2) {
	
	int ts, t, wh = x2-x1, hg = y2-y1;
	lgw = kHudLogoWidth, tw = kHudPanelHeigh;
	t = tw >> 1;
	
	wide = (wh >= 1023);
	slim = (hg == kHudPanelHeigh);
	draw = (wh >= 640 && hg >= kHudPanelHeigh);

	main.x1 = x1;
	main.y1 = y1;
	main.x2 = x2;
	main.y2 = y2;

	if (wide)
	{
		ts = hg-tw-t;
		tilebox.x1 = x2-ts;
		tilebox.y1 = y1+tw;
		tilebox.x2 = x2;
		tilebox.y2 = y2-t;
		
		tileInfo.inside = (WindowInside(&tilebox, &main) && (tilebox.y2 - tilebox.y1) > 80);
	}

	if (!tileInfo.inside)
	{
		ts = mulscale16(gHudPrefs.tileScaleSize, 0xD8F0);
		tilebox.x1 = x2-ts-t;
		tilebox.x2 = x2;
		tilebox.y1 = y1-ts;
		tilebox.y2 = y1+t;
	}
	
	logo.x1 = x1;
	logo.y1 = y1;
	logo.x2 = x1+lgw;
	logo.y2 = y1+tw;
	
	message.x1 = x1+lgw-t;
	message.y1 = y1+t;
	message.x2 = x2;
	message.y2 = y1+tw;
	
	if (wide)
	{
		zoom2d.x1 = x1;
		zoom2d.y1 = y2-t;
		zoom2d.x2 = zoom2d.x1+(lgw>>1);
		zoom2d.y2 = y2;
		
		coords.x1 = zoom2d.x2;
		coords.y1 = y2-t;
		coords.x2 = coords.x1+lgw+(lgw>>1);
		coords.y2 = y2;
		
		grid2d.x1 = coords.x2;
		grid2d.y1 = y2-t;
		grid2d.x2 = grid2d.x1+(lgw>>1);
		grid2d.y2 = y2;
	}
	else
	{
		coords.x1 = x1;
		coords.y1 = y2-t;
		coords.x2 = x1+lgw+(lgw>>2);
		coords.y2 = y2;
	}
	
	status.x1 = (wide) ? grid2d.x2 : coords.x2;
	status.y1 = y2-t;
	status.x2 = x2;
	status.y2 = y2;

	content.x1  = x1+4;
	content.y1  = y1+tw+4;
	content.x2  = (wide) ? tilebox.x1 : x2;
	content.y2  = y2-t;
	
	editHints.x1 = x1;
	editHints.x2 = x2;
	editHints.y1 = y2-t;
	editHints.y2 = y2;
	
	comment.x1 = x1;
	comment.x2 = x2;
	comment.y1 = y2-t;
	comment.y2 = y2;
	
}

void MAPEDIT_HUD::UpdateView(int x1, int y1, int x2, int y2) {
	
	int tmp;
	SetView(x1, y1, x2, y2);
	
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs;
	SetLogo(prefs->header, prefs->charSpace, prefs->icon, prefs->fontId);
	
	MAPEDIT_HUD_TILE* tile =& tileInfo;
	tmp = tileInfo.nTile; tile->nTile = (tile->nTile == -1) ? 0 : -1; // to force update
	SetTile(tmp, tile->nPlu, tile->nShade);
	if (draw)
		DrawIt();
}

void MAPEDIT_HUD::ChangeHeight(int nHeight, char flags)
{
	int x1, y1, x2, y2;
	GetWindowCoords(&main, &x1, &y1, &x2, &y2);
	if (flags & 0x01) UpdateView(x1, y1, x2, y1+nHeight);
	if (flags & 0x02) UpdateView(x1, y2-nHeight, x2, y2);
}

void MAPEDIT_HUD::ChangeWidth(int nWidth, char flags)
{
	int x1, y1, x2, y2;
	GetWindowCoords(&main, &x1, &y1, &x2, &y2);
	if (flags & 0x01) UpdateView(x1+nWidth, y1, x2, y2);
	if (flags & 0x02) UpdateView(x1, y1, x2-nWidth, y2);
}

void MAPEDIT_HUD::SetLogo(char* text, int spacing, int nTileIcon, int fontID) {
	
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs;
	
	if (text && strcmp(prefs->header, text) != 0)
		sprintf(prefs->header, text);
	
	prefs->charSpace 	= spacing;
	prefs->icon 		= nTileIcon;
	prefs->fontId		= fontID;
	prefs->iconScale 	= perc2val(90, tw);
	prefs->iconShade	= 0;
	prefs->iconPlu		= 0;
	prefs->offsx 		= 0;
	prefs->offsy		= 0;
	prefs->iconx 		= 0;
	prefs->icony 		= 0;
	prefs->textx 		= 0;
	prefs->texty 		= 0;
	
	QFONT* pFont = qFonts[prefs->fontId];
	int x1, x2, y1, y2, x3, y3, iw, ih, i, len, fh = pFont->height;
	GetWindowCoords(&logo, &x1, &y1, &x2, &y2);
	
	i = pFont->charSpace;
	if (prefs->charSpace != 255) pFont->charSpace = prefs->charSpace;
	len = gfxGetTextLen(prefs->header, pFont);
	pFont->charSpace = i;
	
	x3 = x1+(((lgw-tw)>>1)-(len>>1));
	if (prefs->icon >= 0)
	{
		tileDrawGetSize(prefs->icon, prefs->iconScale, &iw, &ih);
		prefs->textx = x3+4+(iw>>1);
		prefs->texty = y1+(tw>>1)-(fh>>1);
		
		prefs->iconx = ClipLow(x3-4-(iw>>1), x1);
		prefs->icony = ClipLow(y1+(tw>>1)-(ih>>1), y1);
		
	}
	else
	{
		prefs->textx = x3;
		prefs->texty = y1+(tw>>1)-(fh>>1);
	}
}

void MAPEDIT_HUD::DrawLines(MAPEDIT_HUD_WINDOW* around)
{
	gfxSetColor(colors.line1);
	gfxVLine(around->x1, around->y1, around->y2);
	gfxVLine(around->x2, around->y1, around->y2);
	gfxHLine(around->y1, around->x1, around->x2);
	gfxHLine(around->y2, around->x1, around->x2);
}

void MAPEDIT_HUD::DrawMask() {
	
	int i, x1, x2, y1, y2, t = tw>>1;
	GetWindowCoords(&main, &x1, &y1, &x2, &y2);

	gfxSetColor(colors.mainbg);
	gfxFillBox(x1, y1+tw, x2, y2);
	
	gfxSetColor(colors.line1); gfxVLine(x1, y1, y2);
	gfxSetColor(colors.line2); gfxVLine(x2, y1+t, y2);
	gfxHLine(y2, x1, x2);
	
	GetWindowCoords(&logo, &x1, &y1, &x2, &y2);
	gfxSetColor(colors.line1);	gfxHLine(y1, x1, x2 - tw);
	gfxVLine(x1, y1, y2);
	
	gfxSetColor(colors.line2);
	for (i = 0; i < tw; i++)
		gfxPixel(x2 + i - tw, y1 + i);
	
	GetWindowCoords(&message, &x1, &y1, &x2, &y2);
	gfxSetColor(colors.line1); gfxHLine(y1-1, x1, x2);
	gfxSetColor(colors.line2); gfxHLine(y2, x1 + t, x2);
		
	DrawLines(&coords);
	DrawLines(&status);
	DrawLines(&zoom2d);
	DrawLines(&grid2d);
}

void MAPEDIT_HUD::DrawLogo() {
	
	int x1, x2, y1, y2, x3, i, pic, len;
	MAPEDIT_HUD_LOGO* prefs =& logoPrefs; QFONT* pFont = qFonts[prefs->fontId];
	GetWindowCoords(&logo, &x1, &y1, &x2, &y2);
	
	gfxSetColor(colors.logobg);
	for (i = 0, x3 = x2 - tw - 1; i < tw; i++)
		gfxHLine(y1 + i, x1, x3 + i);
	
	gfxSetColor(colors.line1);
	gfxHLine(y1, x1, x3);
	gfxVLine(x1, y1, y2);
	
	i = pFont->charSpace;
	if (prefs->charSpace != 255)
		pFont->charSpace = prefs->charSpace;
	
	gfxPrinTextShadow(prefs->textx, prefs->texty, colors.logofr, prefs->header, pFont);
	pFont->charSpace = i;
	
	if (prefs->icon >=0)
	{
		pic = tileGetPic(prefs->icon);
		tileDrawTile(prefs->iconx, prefs->icony, pic, prefs->iconScale, prefs->iconPlu, 0x02, tileShade[pic]);
	}
}

void MAPEDIT_HUD::DrawIt() {

	DrawMask();
	DrawLogo();
	PrintMessage();
	
	if (pComment && !slim)
	{
		PrintComment();
		if (wide)
			DrawTile();
	}
	else
	{
		if (!slim )
		{
			PrintCoords();
			PrintStats();
		}
		
		if (wide)
		{
			if (!slim)
			{
				PrintZoom2d();
				PrintGrid2d();
			}
			
			DrawTile();
		}
	}
}

void MAPEDIT_HUD::ClearContent() {
	
	int x1, x2, y1, y2;
	GetWindowCoords(&content, &x1, &y1, &x2, &y2);
	gfxSetColor(colors.mainbg);
	gfxFillBox(x1, y1, x2, y2);
	
}

void MAPEDIT_HUD::PrintComment()
{
	char buf1[256];
	int x1, x2, y1, y2, wh, hg, i, len;
	if (!pComment)
		return;
	
	GetWindowCoords(&comment, &x1, &y1, &x2, &y2);
	wh = x2-x1; hg = y2-y1;	
	
	gfxSetColor(colors.cmtbg);
	gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	
	i = wh>>3;
	if ((len = sprintf(buf1, "%0.255s", pComment->text)) >= i)
		buf1[i] = 0, len = i;
	
	len<<=3;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.cmtfr, buf1);
	
}

void MAPEDIT_HUD::DrawEditDialogHints(DIALOG_ITEM* dialog, DIALOG_ITEM* control) {

	if (!control || !dialog || !gHints)
		return;
	
	BOOL typeHint;
	int x1, x2, y1, y2, wh, hg, i, len;
	char buf1[256], buf2[32], buf3[32], *tmp = NULL;
	
	typeHint = (control->tabGroup == 1 && dialog != dlgXSectorFX);
	sprintf(buf3, "%d", (typeHint) ? dialog->value : control->tabGroup - 1);
	sprintf(buf1, (typeHint) ? "Type" : "Dialog");
	sprintf(buf2, "Hints.%s.", buf1);
	
	if (dialog == dlgXSprite)		 strcat(buf2, gSearchStatNames[OBJ_SPRITE]);
	else if (dialog == dlgXSector)	 strcat(buf2, gSearchStatNames[OBJ_SECTOR]);
	else if (dialog == dlgXWall)	 strcat(buf2, gSearchStatNames[OBJ_WALL]);
	else if (dialog == dlgXSectorFX) strcat(buf2, "SectorFX");
	else return;
	
	if (strlen(tmp = gHints->GetKeyString(buf2, buf3, "")) == 0)
	{
		sprintf(buf2, "Hints.%s.Shared", buf1);
		tmp = gHints->GetKeyString(buf2, buf3, "<no information found>");
	}
	
	GetWindowCoords(&editHints, &x1, &y1, &x2, &y2);
	wh = x2-x1; hg = y2-y1;	
		
	gfxSetColor(colors.hintbg);
	gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	
	i = wh>>3;
	if ((len = sprintf(buf1, "%0.255s", tmp)) >= i)
		buf1[i] = 0, len = i;
	
	len<<=3;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.hintfr, buf1);
}

void MAPEDIT_HUD::ClearMessageArea()
{
	int x1, x2, y1, y2, x3, i, t = tw >> 1;
	GetWindowCoords(&message, &x1, &y1, &x2, &y2);
	
	gfxSetColor(colors.mesgbg);
	for (i = 0, x3 = x1+1; i < t; i++)
		gfxHLine(y1+i, x3+i, x2);
}

void MAPEDIT_HUD::SetMsgImp(int nTime, char *__format, ...) {
	
	msgData.message[0] = '\0';

	va_list argptr;
	va_start(argptr, __format);
	vsprintf(msgData.message, __format, argptr);
	va_end(argptr);
	
	msgData.ticks = totalclock + nTime;
}

void MAPEDIT_HUD::SetMsg(char *__format, ...)
{
	if (msgData.ticks >= 0 && totalclock >= msgData.ticks)
	{
		msgData.ticks = 0;
		msgData.message[0] = '\0';
		
		va_list argptr;
		va_start(argptr, __format);
		vsprintf(msgData.message, __format, argptr);
		va_end(argptr);
	}
}

void MAPEDIT_HUD::SetMsg()
{
	if (msgData.ticks >= 0 && totalclock >= msgData.ticks)
	{
		msgData.ticks = 0;
		msgData.message[0] = '\0';
	}
}

void MAPEDIT_HUD::PrintMessage()
{
	char tcol;
	int x1, x2, y1, y2, hg, t = tw >> 1;
	MAPEDIT_HUD_MSG* pData =& msgData;
	tcol = (pData->ticks && h) ? colors.mesgfr2 : colors.mesgfr1;

	ClearMessageArea();
	GetWindowCoords(&message, &x1, &y1, &x2, &y2); hg = y2 - y1;
	printextShadowL(x1 + t, y1+((hg>>1)-(8>>1)), tcol, pData->message);
}

void MAPEDIT_HUD::PrintCoords() {
	
	char tmp[64];
	int x1, x2, y1, y2, len, wh, hg;
	GetWindowCoords(&coords, &x1, &y1, &x2, &y2);
	if (wide)
		len = sprintf(tmp, "x:%-6d y:%-6d z:%-6d ang:%-4d", posx, posy, posz, ang);
	else
		len = sprintf(tmp, "x:%-6d y:%-6d ang:%-4d", posx, posy, ang);
	
	if (colors.cordbg != colors.mainbg)
	{
		gfxSetColor(colors.cordbg);
		gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	}
	
	len *= 8; wh = x2 - x1, hg = y2 - y1;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.cordfr, strupr(tmp));
}


void MAPEDIT_HUD::PrintZoom2d() {
	
	char tmp[64];
	int x1, x2, y1, y2, len, wh, hg;
	GetWindowCoords(&zoom2d, &x1, &y1, &x2, &y2);
	
	if (colors.zoombg != colors.mainbg)
	{
		gfxSetColor(colors.zoombg);
		gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	}
	
	wh = x2 - x1, hg = y2 - y1;
	if (qsetmode != 200)
		len = sprintf(tmp, "zoom:%d", zoom);
	else
		len = sprintf(tmp, "%s", gZModeNames[zmode]);
	
	len<<=3;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.zoomfr, strupr(tmp));
	
}

void MAPEDIT_HUD::PrintGrid2d() {
	
	char tmp[64];
	int nGrid = grid, x1, x2, y1, y2, len, wh, hg;
	GetWindowCoords(&grid2d, &x1, &y1, &x2, &y2);
	
	if (colors.gridbg != colors.mainbg)
	{
		gfxSetColor(colors.gridbg);
		gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	}
	
	if (qsetmode == 200 && gMouse.fixedGrid)
		nGrid = gMouse.fixedGrid;
	
	wh = x2 - x1, hg = y2 - y1;
	len = sprintf(tmp, "grid:%d", nGrid) << 3;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.gridfr, strupr(tmp));
	
}

void MAPEDIT_HUD::PrintStats() {
	
	char tmp[128];
	int x1, x2, y1, y2, len, wh, hg;
	GetWindowCoords(&status, &x1, &y1, &x2, &y2);
	BOOL xstats = (qsetmode != 200 && keystatus[KEY_CAPSLOCK]);
		
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
	
	if (colors.statbg != colors.mainbg)
	{
		gfxSetColor(colors.statbg);
		gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	}
	
	len*=8; wh = x2 - x1, hg = y2 - y1;
	printextShadowL(x1+((wh>>1)-(len>>1)), y1+((hg>>1)-(8>>1)), colors.statfr, strupr(tmp));
}

void MAPEDIT_HUD::DrawTile()
{
	char buf[32];
	int nTile, nPlu, nShade, tis, r;
	int x, y, x1, x2, y1, y2, hmr, ln;
	GetWindowCoords(&tilebox, &x1, &y1, &x2, &y2);

	nTile = tileInfo.nTile; nPlu = tileInfo.nPlu; nShade = tileInfo.nShade;
	tis   = tileInfo.size;
	hmr   = tileInfo.marg1>>1;
	r 	  = tileInfo.marg2;
	
	if (!tileInfo.inside)
	{
		gfxSetColor(colors.line2);
		gfxFillBox(x1+r, y1+r, x2-r, y2-r);
	}
	
	x = x1+hmr; y = y1+hmr;
	gfxSetColor(colors.tilbg);
	gfxFillBox(x, y, x+tis, y+tis);
	gfxVLine(x-r, y-r, y+tis+r);	gfxVLine(x+tis+r, y-r, y+tis+r);
	gfxHLine(y-r, x-r, x+tis+r);	gfxHLine(y+tis+r, x-r, x+tis+r);

	gfxSetColor(colors.line2);
	gfxVLine(x+(tis>>1), y+r, y+tis-r);
	gfxHLine(y+(tis>>1), x+r, x+tis-r);
	
	if (nTile >= 0)
	{
		x = tileInfo.dx; y = tileInfo.dy;
		if (tilesizx[nTile])
		{
			//tileDrawTile(x+1, y+1, nTile, tileInfo.size, 0, 0x02, 32); // shadow
			tileDrawTile(x, y, nTile, tileInfo.size, nPlu, 0x02, nShade);
		}
		else
		{
			printextShadowL(x, y, colors.tilblk, "<BLANK>");
		}
		
		x = x1+hmr; y = y1+hmr;
		sprintf(buf, "%d", nTile);
		printextShadowL(x, y, colors.tilnum, buf);
		
		if (nPlu)
		{
			sprintf(buf, "%d", nPlu);
			printextShadowL(x, y+8, colors.tilplu, buf);
		}
		
		if (nShade)
		{
			ln = sprintf(buf, "%d", nShade)<<3;
			printextShadowL(x+tis-ln, y, colors.tilshd, buf);
		}
		
		y += tis-6;
		ln = sprintf(buf, surfNames[surfType[nTile]])<<2;
		printextShadowS(x, y, colors.tilsrf, strupr(buf));
		
		ln = sprintf(buf, "%dx%d", tilesizx[nTile], tilesizy[nTile])<<2; x += tis-ln;
		printextShadowS(x, y, colors.tilsiz, buf);
		
	}
}

void MAPEDIT_HUD::SetTile(int nTile, int nPlu, int nShade)
{
	int nTile2, x, y, x1, x2, y1, y2, tis, tiw, tih, mr, hmr, ln, wh, hg;
	GetWindowCoords(&tilebox, &x1, &y1, &x2, &y2);
	wh = x2-x1, hg = y2-y1; nTile2 = tileInfo.nTile;
	
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
	
	tis = hg;
	if ((mr = perc2val(tis, 20)) % 2 != 0)
		mr--;
	
	tis -= mr; hmr = mr>>1;
		
	tileInfo.size  = tis;
	tileInfo.marg1 = mr;
	tileInfo.marg2 = perc2val(tis, 6);

	if (nTile >= 0)
	{
		if (tilesizx[nTile])
		{
			tileDrawGetSize(nTile, tis, &tiw, &tih);
			tileInfo.dx = x1+abs((wh>>1)-(tiw>>1));
			tileInfo.dy = y1+abs((hg>>1)-(tih>>1));
		}
		else
		{
			ln = strlen("<BLANK>")<<3;
			tileInfo.dx = x1+hmr+abs((tis>>1)-(ln>>1));
			tileInfo.dy = y1+hmr+abs((tis>>1)-(8>>1));
		}
	}
}

void MAPEDIT_HUD::SetComment(MAP_COMMENT* cmt)
{
	pComment = cmt;
}

void hudSetLayout(MAPEDIT_HUD* pHud, int layoutType, MOUSE* pMouse)
{
	int hg, x1, y1, x2, y2;
	switch (layoutType) {
		case kHudLayoutNone:
			x1 = 0;
			y1 = 0;
			x2 = 0;
			y2 = 0;
			break;
		case kHudLayoutFull:
		case kHudLayoutDynamic:
			hg = kHudHeighNormal;
			x1 = 0;	
			y1 = ydim-hg;
			x2 = xdim;
			y2 = ydim;
			break;
		case kHudLayoutCompact:
			hg = kHudHeighCompact;
			x1 = 0;	
			x2 = xdim;
			y1 = ydim-hg;
			y2 = ydim;
			break;
		case kHudLayoutSlim:
			hg = kHudHeighSlim;
			x1 = 0;	
			y1 = ydim-hg-1;
			x2 = xdim;
			y2 = ydim-1;
			break;
		default:
			return;
	}

	pHud->UpdateView(x1, y1, x2, y2);
	//ydim16 = ydim;
	ydim16 = (y1 <= 0 && y2 == y1) ? ydim : pHud->main.y1+pHud->tw;
	
	if (pMouse)
	{
		if (!pHud->draw) pMouse->SetRange(0, 0, xdim, ydim);
		else pMouse->SetRange(0, 0, xdim, pHud->main.y1);
	}
	
}
