/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to convert various images to the ART graphics format.
// It also contains functions to work with palette formats.
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


#ifndef __IMG2TILE_H
#define __IMG2TILE_H

#include "screen.h"
#include "gfx.h"

//#define I2TDEBUG 1
#define kMaxImageWidth 640
#define kMaxImageHeight 640

#pragma pack(push, 1)
struct NAMED_TYPE;
struct PCX_HEADER {
	
	char manufacturer;
	char version;
	char encoding;
	unsigned char bitsPerPixel;
	short x0, y0, x1, y1;
	short xDPI, yDPI;
	RGB pal16[16];
	char reserved;
	char planes;
	short bytesPerLine;
	short paletteInfo;
	short xScreenSize, yScreenSize;
	char filler[54];
};

struct TGA_HEADER {
	
	uint8_t		idLength;		/* length of ID string */
	uint8_t		mapType;		/* color map type */
	uint8_t		imageType;		/* image type code */
	uint16_t	mapOrigin;		/* starting index of map */
	uint16_t	mapLength;		/* length of map */
	uint8_t		mapWidth;		/* width of map in bits */
	uint16_t	xOrigin;		/* x-origin of image */
	uint16_t	yOrigin;		/* y-origin of image */
	uint16_t	imageWidth;		/* width of image */
	uint16_t	imageHeight;	/* height of image */
	uint8_t		pixelDepth;		/* bits per pixel */
	uint8_t		imageDesc;		/* image descriptor */
	
};

struct QBM_HEADER {
    uint8_t type, tcolor;
    uint16_t width, height, bpl;
    uint16_t xofs, yofs;
};
#pragma pack(pop)


enum {
kImageQBM		= 0,
kImagePCX		= 1,
kImagePCC		= 2,
kImageTGA		= 3,
kImageMax		   ,
};

enum {
kPaletteDAT			= 0,
kPalettePAL			= 1,
kPaletteMicrosoft	= 2,
kPalettePaintShop	= 3,
kPalettePLU			= 4,
kPaletteMax		   	   ,	
};

extern NAMED_TYPE gSuppImages[kImageMax];
extern NAMED_TYPE gSuppPalettes[kPaletteMax];
extern NAMED_TYPE gImgErrorsCommon[];
extern NAMED_TYPE gPalErrorsCommon[];


int imgGetType(char* str);
int palGetType(char* str);
int palLoad(char* fname, PALETTE out);
int pluLoad(char* fname, PALETTE out, int nPlu = 1, int nShade = 0);
int getTypeByExt(char* str, NAMED_TYPE* db, int len);

int tga2tile(char* pBuf, int bufLen, int nTile);
int tga2tile(char* filepath, int nTile);
int tile2tga(char* filepath, int nTile);

int pcx2tile(char* filepath, int nTile);
int tile2pcx(char* img, int nTile);

int qbm2tile(char* filepath, int nTile);
int tile2qbm(char* img, int nTile);

int rawImg2Tile(BYTE* image, int nTile, int wh, int hg, int align = 1);
BOOL palFixTransparentColor(PALETTE imgPal);
BOOL BuildPLU(BYTE* out, PALETTE pal, int grayLevel = 0);
BOOL BuildPLU(char *filename, PALETTE palette, int grayLevel = 0);
void remapColors(intptr_t image, int len, PALETTE srcPal);
void bufRemap(BYTE* buf, int len, BYTE* table);
BYTE countBestColor(PALETTE in, int r, int g, int b, int wR = 1, int wG = 1, int wB = 1);
#endif