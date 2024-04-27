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

#ifndef __XMP2DSCR
#define __XMP2DSCR
#include "db.h"
#include "screen.h"

class SCREEN2D
{
	public:
		char colors[32][2];
		// ---------------------------------------------------------------------
		spritetype* pSpr; XSPRITE* pXSpr;
		unsigned int numflines;
		LINE2D* flines;
		QFONT* pCaptFont;
		QFONT* pEditFont;
		// ---------------------------------------------------------------------
		unsigned int HOVER		: 1;
		unsigned int BLINK		: 1;
		unsigned int THICK		: 1;
		unsigned int color		: 8;
		unsigned int vertexSize	: 8;
		unsigned int gridSize	: 32;
		signed   int x1, x2, x3;
		signed   int y1, y2, y3;
		// ---------------------------------------------------------------------
		struct
		{
			int wwh, whg;
			int wx1, wy1;
			int wx2, wy2;
			int wcx, wcy;
		}
		view;
		struct
		{
			unsigned int useTransluc		: 1;
			unsigned int ambRadius			: 4;
			unsigned int ambRadiusHover		: 1;
			unsigned int showSprites		: 4;
			unsigned int showFloorBorders	: 1;
			unsigned int showVertex			: 1;
			unsigned int showMap			: 4;
			unsigned int showHighlight		: 1;
			unsigned int showTags			: 3;
			unsigned int showFeatures		: 1;
			unsigned int clipView			: 1;
		}
		prefs;
		struct
		{
			unsigned int sprUnderCursor		: 16;
			unsigned int grid				: 4;
			unsigned int ang 				: 12;
			int camx, camy;
			int zoom;
		}
		data;
		SCREEN2D()
		{
			memset(colors, 255, sizeof(colors));
			SetView(0, 0, 0, 0);
			flines = NULL;
			data.zoom = 768;
		}
		~SCREEN2D()
		{
			if (flines)
				free(flines);
			
			numflines = 0;
		}
		// ---------------------------------------------------------------------
		void SetView(int x1, int y1, int x2, int y2, char clip = 0);
		// ---------------------------------------------------------------------
		void ColorInit(IniFile* pIni);
		void ColorFill(char which, char color, int nStart);
		char ColorGet(char which, char hover = 0);
		char GetWallColor(int nWall, char hover);
		char GetSpriteColor(spritetype* pSpr, char hover);
		// ---------------------------------------------------------------------
		void ScreenDraw(void);
		void ScreenClear(void);
		inline void ScreenDraw(int camx, int camy, int ang, int grid, int zoom)
		{
			data.ang		= ang;
			data.grid		= grid;
			data.zoom		= zoom;
			data.camx		= posx;
			data.camy		= posy;
			
			ScreenDraw();
		}
		// ---------------------------------------------------------------------
		void DrawGrid(void);
		void DrawLine(int x1, int y1, int x2, int y2, char c, char b, int drawPat = kPatNormal);
		void DrawLine(int x1, int y1, int x2, int y2, char nColor);
		void DrawAngLine(int nScale, int nAng, int x1, int y1, char c, char b = false);
		void DrawIconFaceSpr_SMALL(int x, int y, char c);
		void DrawIconFaceSpr_NORMAL(int x, int y, char c);
		void DrawIconSpr(spritetype* pSpr, int x, int y, char c);
		void DrawIconMarker(int x, int y, char c);
		void DrawIconCross(int x, int y, char c, char s = 2);
		void DrawIconCross(int x, int y, int nAng, char c, char s = 2);
		void DrawCircle(int x, int y, int r, char c, char b = 0, int drawPat = kPatNormal);
		void DrawRect(int x1, int y1, int x2, int y2, char c, char b = 0, int drawPat = kPatNormal);
		void DrawSquare(int x, int y, char bc, int s);
		void DrawVertex(int x, int y, char bc, char fc, char s = 4);
		void DrawArrow(int xS, int yS, int xD, int yD, char c, int nZoom, int nAng = kAng30, char b = 0, int pat1 = kPatNormal, int pat2 = kPatNormal);
		void DrawArrow(int x, int y, char c, int nAng, int nZoom, int nSiz = 80, char b = false);
		// ---------------------------------------------------------------------
		void DrawSprite(spritetype* pSprite);
		void DrawSpritePathMarker(void);
		void DrawSpriteMarker(void);
		void DrawSpriteAmbient(void);
		void DrawSpriteStealth(void);
		void DrawSpriteFX(void);
		void DrawSpriteExplosion(void);
		void DrawSpriteProjectile(void);
		void DrawSpritePlayer(void);
		void DrawSpriteCommon(void);
		// ---------------------------------------------------------------------
		void DrawWall(int nWall);
		void DrawWallMidPoint(int nWall, char c, char s = 5, char thick = 0, char which = 0x01);
		void MarkWall(int nWall, int nMarkType);
		// ---------------------------------------------------------------------
		const char* CaptionGet(int nType, int nID);
		void CaptionPrint(const char* text, int cx, int cy, char pd, char fc, short bc, char shadow, QFONT* pFont);
		void CaptionPrintSector(int nSect, char hover, QFONT* pFont);
		void CaptionPrintSprite(int nSpr, char hover, QFONT* pFont);
		void CaptionPrintWall(int nWall, char hover, QFONT* pFont);
		void CaptionPrintLineEdit(int nOffs, int x1, int y1, int x2, int y2, char fc, char bc, QFONT* pFont);
		void CaptionPrintWallEdit(int nWall, char fc, char bc, QFONT* pFont);
		// ---------------------------------------------------------------------
		void FillSector(int nSect, char c, char nStep = 2, int drawPat = kPatNormal);
		void FillPolygon(LINE2D* coords, int nCount, char c, char nStep = 2, int drawPat = kPatNormal);
		// ---------------------------------------------------------------------
		void ShowMap(char flags = 0x04);
		// ---------------------------------------------------------------------
		void GetPoint(int scrX, int scrY, int* posX, int* posY, char clamp);
		void GetPoint(int scrX, int scrY, int* posX, int* posY);
		void GetPoint(int* x, int* y);
		// ---------------------------------------------------------------------
		void LayerOpen(void);
		void LayerShow(char flags = 0);
		void LayerShowAndClose(char flags = 0);
		void LayerClose(void);
		// ---------------------------------------------------------------------
		char InHighlight(int x, int y);
		void ScaleAngLine(int scale, int ang, int* x, int* y);
		void ScalePoints(int* x1 = NULL, int* y1 = NULL, int* x2 = NULL, int* y2 = NULL);
		char OnScreen(int x1, int y1, int x2, int y2);
		
		
		inline char OnScreen(int x, int y, int s)	{ return OnScreen(x-s, y-s, x+s, y+s); }
		inline int cscalex(int a)				 	{ return view.wcx + mulscale14(a - data.camx, data.zoom); }
		inline int cscaley(int a)				 	{ return view.wcy + mulscale14(a - data.camy, data.zoom); }
};

#endif