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

#ifndef __XMPHUD_H
#define __XMPHUD_H
#include "gfx.h"
#include "mapcmt.h"
#include "xmphudlg.h"
#include "xmpconf.h"

#define kHudPanelHeigh		32
#define kHudLogoWidth		200
#define kHudHeighNormal 	144
#define kHudHeighCompact 	kHudPanelHeigh+(kHudPanelHeigh>>1)
#define kHudHeighSlim		kHudPanelHeigh

enum {
kHudLayoutNone			= 0,
kHudLayoutFull			= 1,
kHudLayoutCompact		= 2,
kHudLayoutSlim			= 3,
kHudLayoutDynamic		= 4,
kHudLayoutMax		   	   ,
};

extern char* gHudLayoutNames[kHudLayoutMax];

struct MAPEDIT_HUD_COLORS {
	
	char mainbg;
	char logobg,	logofr;
	char cordbg,	cordfr;
	char statbg,	statfr;
	char mesgbg,	mesgfr1,	mesgfr2;
	char hintbg,	hintfr;
	char line1,		line2;
	char zoombg,	zoomfr;
	char gridbg,	gridfr;
	char tilnum,	tilplu;
	char tilsiz,	tilsrf;
	char tilblk,	tilbg,		tilshd;
	char cmtbg,		cmtfr;
	
};

struct MAPEDIT_HUD_WINDOW {
	
	unsigned int x1				: 14;
	unsigned int y1				: 14;
	unsigned int x2				: 14;
	unsigned int y2				: 14;
	
};

struct MAPEDIT_HUD_LOGO {
	
	unsigned int fontId			:  8; 
	char header[32];
	signed int 	 charSpace		:  8;
	signed int	 icon			: 16;
	signed int   iconShade 		:  8;
	unsigned int iconScale		: 10;
	unsigned int iconPlu		:  8;
	unsigned int textx			: 14;
	unsigned int texty			: 14;
	unsigned int iconx			: 14;
	unsigned int icony			: 14;
	unsigned int offsx			:  8;
	unsigned int offsy			:  8;
	
};

struct MAPEDIT_HUD_MSG {
	
	char message[256];
	signed   int ticks;
	
};

struct MAPEDIT_HUD_TILE
{
	signed   int nTile			: 16;
	unsigned int nPlu			: 8;
	signed   int nShade			: 8;
	unsigned int inside			: 1;
	unsigned int size			: 10;
	unsigned int dx				: 14;
	unsigned int dy				: 14;
	unsigned int marg1			: 8;
	unsigned int marg2			: 8;
	
};

/* struct MAPEDIT_HUD_DIALOG {
	
	DIALOG_ITEM* dialog;
	DIALOG_ITEM* control;
	void Edit();
	void Draw();
	void DrawItem();
	
}; */

struct MAPEDIT_HUD {
	
	unsigned int draw	:  1;
	unsigned int wide	:  1;
	unsigned int slim	:  1;
	unsigned int lgw	: 10;
	unsigned int tw		: 10;
	MAPEDIT_HUD_WINDOW main;
	MAPEDIT_HUD_WINDOW message;
	MAPEDIT_HUD_WINDOW content;
	MAPEDIT_HUD_WINDOW coords;
	MAPEDIT_HUD_WINDOW status;
	MAPEDIT_HUD_WINDOW tilebox;
	MAPEDIT_HUD_WINDOW logo;
	MAPEDIT_HUD_WINDOW editHints;
	MAPEDIT_HUD_WINDOW grid2d;
	MAPEDIT_HUD_WINDOW zoom2d;
	MAPEDIT_HUD_WINDOW comment;
	//MAPEDIT_HUD_WINDOW keystatus;
	MAPEDIT_HUD_COLORS colors;
	MAPEDIT_HUD_LOGO logoPrefs;
	MAPEDIT_HUD_MSG msgData;
	MAPEDIT_HUD_TILE tileInfo;
	//MAPEDIT_HUD_DIALOG dialog;
	MAP_COMMENT* pComment;
	void InitColors();
	BOOL WindowInside(MAPEDIT_HUD_WINDOW* w1, MAPEDIT_HUD_WINDOW* w2);
	void SetView(int x1, int y1, int x2, int y2);
	void UpdateView(int x1, int y1, int x2, int y2);
	void ChangeHeight(int nHeight, char flags = 0x02);
	void ChangeWidth(int nWidth, char flags = 0x02);
	void SetLogo(char* text = NULL, int spacing = 255, int nTileIcon = -1, int fontID = 0);
	void SetTile(int nTile = -1, int nPlu = 0, int nShade = 0);
	void SetComment(MAP_COMMENT* cmt = NULL);
	void SetMsg();
	void SetMsg(char *__format, ...);
	void SetMsgImp(int nTime, char *__format, ...);
	void GetWindowCoords(MAPEDIT_HUD_WINDOW* from, int *ox1, int *oy1, int *ox2, int *oy2);
	void GetWindowSize(MAPEDIT_HUD_WINDOW* from, int *ow = NULL, int *oh = NULL);
	void DrawIt();
	void DrawMask();
	void DrawLogo();
	void DrawLines(MAPEDIT_HUD_WINDOW* around);
	void DrawEditDialogHints(DIALOG_ITEM* dialog, DIALOG_ITEM* control);
	void DrawTile();
	void ClearMessageArea();
	void ClearContent();
	void PrintGrid2d();
	void PrintZoom2d();
	void PrintMessage();
	void PrintCoords();
	void PrintStats();
	void PrintComment();
	
};

extern MAPEDIT_HUD gMapedHud;
void hudSetLayout(MAPEDIT_HUD* pHud, int layoutType, MOUSE* pMouse = NULL);

#endif