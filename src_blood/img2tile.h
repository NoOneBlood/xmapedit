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
#define USE_KPLIB 1
#define kMaxImageWidth 32768
#define kMaxImageHeight 32768

typedef int (*IMG2TILEFUNC)(unsigned char* pBuf, int bufLen, int nTile);

#pragma pack(push, 1)
struct PCX_HEADER {

	int8_t manufacturer;
	int8_t version;
	int8_t encoding;
	uint8_t bitsPerPixel;
	int16_t x0, y0, x1, y1;
	int16_t xDPI, yDPI;
	RGB pal16[16];
	int8_t reserved;
	int8_t planes;
	int16_t bytesPerLine;
	int16_t paletteInfo;
	int16_t xScreenSize, yScreenSize;
	int8_t filler[54];
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

struct CEL_HEADER {
	
	uint16_t magic;
	uint16_t width, height;
	uint16_t xoffs, yoffs;
	uint8_t  bpp, compression;
	uint32_t bytes;
	uint8_t reserved[16];
};

#pragma pack(pop)


enum {
kImageQBM		= 0,
kImagePCX		= 1,
kImagePCC		= 2,
kImageTGA		= 3,
kImageCEL		= 4,
#ifdef USE_KPLIB
kImageBMP		= 5,
kImageDDS		= 6,
kImagePNG		= 7,
kImageJPG		= 8,
#endif
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
int imgFile2TileFunc(IMG2TILEFUNC img2tileFunc, char* filepath, int nTile);
IMG2TILEFUNC imgGetConvFunc(int nImgType);
int qbm2tile(unsigned char* pBuf, int bufLen, int nTile);
int cel2tile(unsigned char* pBuf, int bufLen, int nTile);
int pcx2tile(unsigned char* pBuf, int bufLen, int nTile);
int tga2tile(unsigned char* pBuf, int bufLen, int nTile);
int png2tile(unsigned char* pBuf, int bufLen, int nTile);
int bmp2tile(unsigned char* pBuf, int bufLen, int nTile);
int dds2tile(unsigned char* pBuf, int bufLen, int nTile);
int jpg2tile(unsigned char* pBuf, int bufLen, int nTile);


int palGetType(char* str);
int palLoad(char* fname, PALETTE out);
void palShift(PALETTE pal, int by = 2);
int pluLoad(char* fname, PALETTE out, int nPlu = 1, int nShade = 0);




int tile2tga(char* filepath, int nTile);
int tile2pcx(char* filepath, int nTile);
int tile2qbm(char* filepath, int nTile);

int convertPixels(BYTE* pixels, int bufLen, int pxsize, char rgbOrder = 0);
#ifdef USE_KPLIB
int kplibImg2Tile(unsigned char* pBuf, int bufLen, int nTile);
#endif
int rawImg2Tile(BYTE* image, int nTile, int wh, int hg, int align = 1);
BOOL palFixTransparentColor(PALETTE imgPal);
BOOL BuildPLU(BYTE* out, PALETTE pal, int grayLevel = 0);
BOOL BuildPLU(char *filename, PALETTE palette, int grayLevel = 0);
void remapColors(intptr_t image, int len, PALETTE srcPal);
void bufRemap(BYTE* buf, int len, BYTE* table);
BYTE countBestColor(PALETTE in, int r, int g, int b, int wR = 1, int wG = 1, int wB = 1);
#endif