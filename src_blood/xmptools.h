/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions required for intergrated editors such as artedit or seqedit.
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

#ifndef __XMPTOOLS_H
#define __XMPTOOLS_H

#include "gui.h"
#include "trig.h"

extern char* toolMenu[3];

struct ARTFILE;
struct POINT {
	
	int x, y;
	
};

#include "xmparted.h"

struct XMPTOOL {
	
	spritetype* pSprite;
	char   objType;
	short  objIndex;
	schar  shade;
	BYTE  surface;
	BOOL   cantest;
	QFONT* pFont;
	ushort hudPixels;
	ushort centerPixels;
	int nOctant;
	int tileZoom;
	int zTop;
	int zBot;
	
};

extern XMPTOOL gTool;

void toolGetResTableValues();
int toolGetDefTileZoom();
void toolDisplayMessage(short nColor, int x, int y, QFONT* pFont);
void toolDrawPixels(int dist = 2, int color = 8);
void toolDrawCenter(POINT* origin, int nColor, int xrng = 6, int yrng = 6, int skip = 4);
void toolDrawCenterHUD(POINT* origin, int nTile, int nVTile, int scrYofs, int nOctant, int nColor);
void toolSetOrigin(POINT* origin, int x, int y );
void toolDrawWindow(int x1, int y1, int x2, int y2, char* title, char textColor);
int toolGetViewTile(int nTile, int nOctant, char *flags, int *ang);
int toolAskSaveChanges(char* text);
BOOL toolLoadAs(char* path, char* ext, char* title = "Load file");
BOOL toolSaveAs(char* path, char* ext);

void cpyRestoreObject(int type, int idx);
void cpySaveObject(int type, int idx);

int toolExploderSeq();
int toolGibTool(int nGib, int type = -1);
int toolXWalls2XSprites();

int updViewAngle(spritetype* pSprite);
void chgSpriteZ(spritetype* pSprite, int zVal);
void mapImportArtDlg(char* artPath);
int mapImportArt(ARTFILE* files, int nFiles, PALETTE pal, int nStartTile, int flags);
int toolOpenWith(char* filename, char flags = 0x0);
//int toolGetPluDlg(int nTile, int nDefaultRetn = -1);
#endif