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
#pragma once
#include "common_game.h"
#pragma pack(push, 1)

#define kMaxQFonts 13
#define kMaxVFonts 32

// font types
enum
{
kFontTypeMono					= 0,
kFontTypeRasterHoriz			= 1,
kFontTypeRasterVert				= 2,
};

// qbm types
enum
{
kQBitmapRAW 					= 0,
kQBitmapTRAW 					= 1,
kQBitmapRLE 					= 2,
};



enum {
kBitmapLookUp 					= 0,
kBitmapLookDown 				= 1,
kBitmapTurnLeft 				= 2,
kBitmapTurnRight				= 3,
kBitmapDisk1 					= 4,
kBitmapMouseCursor  			= 5,
kBitmapScrollUp					= 6,
kBitmapScrollDn					= 7,
kBitmapSeqeditBg				= 8,
kMaxBitmaps						= 16,
};

struct QBITMAP
{
    unsigned char type;
    unsigned char tcolor;
    unsigned short width;
    unsigned short height;
    unsigned short bpl;
    unsigned short at8;
    unsigned short ata;
    char data[1];
};

typedef struct
{
    int offset;
    unsigned char w;
    unsigned char h;
    unsigned char ox;
    signed char oy;
} QFONTCHAR;

struct QFONT
{
    unsigned char sign[4];
    unsigned short ver;
    unsigned short type;
    unsigned int size;
	unsigned char charFirst;
	unsigned char charLast;
	unsigned char charLead;
    unsigned char baseline;
    unsigned char tcolor;
    signed char charSpace;
    unsigned char width;
    unsigned char height;
    unsigned char unused[12];
    QFONTCHAR info[256];
    char data[1];
};

struct ROMFONT
{
	BYTE* data;
	unsigned int wh			: 5;
	unsigned int hg			: 5;
	signed   int ls			: 6; 
	int GetLength(char* str, int s = 255) { return strlen(str) * ((s == 255) ? ls : s); }
};

#pragma pack(pop)

extern QFONT* qFonts[kMaxQFonts];
extern ROMFONT vFonts[kMaxVFonts];
extern QBITMAP* pBitmaps[kMaxBitmaps];
extern char gColor;


void gfxDrawBitmap(QBITMAP* qbm, int x, int y);
void gfxDrawBitmap(int id, int x, int y);
void gfxPixel(int x, int y);
void gfxHLine(int y, int x0, int x1);
void gfxVLine(int x, int y0, int y1);
void gfxFillBox(int x0, int y0, int x1, int y1);
void gfxSetClip(int x0, int y0, int x1, int y1);
int gfxGetTextLen(char* pzText, QFONT* pFont, int a3 = 0);
int gfxGetLabelLen(char* pzLabel, QFONT* pFont);
int gfxFindTextPos(char* pzText, QFONT* pFont, int a3);
void gfxDrawText(int x, int y, int color, char* pzText, QFONT* pFont = NULL, bool label = false);
void gfxDrawText(int x, int y, int fr, int bg, char* txt, QFONT* pFont = NULL, bool label = false);
void gfxDrawLabel(int, int, int, char*, QFONT* pFont = NULL);
void viewDrawText(int x, int y, QFONT* pFont, char *string, int shade = 0, int nPLU = 0, int nAlign = 0);
void viewDrawChar( QFONT *pFont, BYTE c, int x, int y, BYTE *pPalookup );
void gfxSetColor(char color);
void printext2(int x, int y, char fr, char* text, ROMFONT* pFont, char flags = 0x0);
