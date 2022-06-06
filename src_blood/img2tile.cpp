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

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>

#include "iob.h"
#include "xmpstub.h"
#include "img2tile.h"
#include "screen.h"
#include "tile.h"
#include "inifile.h"
#include "enumstr.h"
#include "gui.h"
#include "common_game.h"
#include "misc.h"

#include "xmparted.h"
#include "xmpconf.h"
#include "xmpmisc.h"

#ifdef USE_KPLIB
extern "C" {
#include "kplib.h"
}
#endif

#define kPluAreaLenOld 31872 // filesize-maxsize-palette
#define kPluAreaLenNew 64766 // filesize-maxsize-numlookups-palette

#define kRiffPalSig "PAL data"
#define kPaintShopPalSig "0100"
#define kTargaV20Sig "TRUEVISION-XFILE."

enum {
kTokenPAL		= 0,
kTokenRGB		= 1,
};


NAMED_TYPE palDBTokens[] = {

	{kTokenPAL, "PAL"},
	{kTokenRGB, "RGB"},

};

NAMED_TYPE gSuppPalettes[] = {
	
	{kPaletteDAT, 		"DAT"},
	{kPalettePAL, 		"PAL"},
	{kPaletteMicrosoft, gSuppPalettes[kPalettePAL].name},
	{kPalettePaintShop, gSuppPalettes[kPalettePAL].name},
	{kPalettePLU, 		"PLU"},
	
};


NAMED_TYPE gPALSigns[] = {
	
	{kPaletteMicrosoft, "RIFF"},
	{kPalettePaintShop,	"JASC-PAL"},
	
};

NAMED_TYPE gSuppImages[] = {

	{kImageQBM, "QBM"},
	{kImageCEL, "CEL"},
	{kImagePCX, "PCX"},
	{kImagePCX, "PCC"},
	{kImageTGA,	"TGA"},
#ifdef USE_KPLIB
	{kImagePNG, "BMP"},
	{kImageDDS, "DDS"},
	{kImageJPG, "JPG"},
	{kImagePNG, "PNG"},
#endif
};

FUNCT_LIST gImgFuncs[] = {
	
	{kImageQBM, qbm2tile},
	{kImageCEL, cel2tile},
	{kImagePCX, pcx2tile},
	{kImageTGA, tga2tile},
#ifdef USE_KPLIB
	{kImageBMP, bmp2tile},
	{kImagePNG, png2tile},
	{kImageDDS, dds2tile},
	{kImageJPG, jpg2tile},
#endif
	
};

NAMED_TYPE gImgErrorsCommon[] = {

	{-1, "Error opening file"},
	{-2, "Incorrect image resolution"},
	{-3, "Cannot allocate tile"},
	{-4, "Image is corrupted"},
	{-5, "Unsupported image format"},
	{-6, "Image is compressed"},
	{-7, "Image have no palette"},
	{-8, "Max image width is exceed"},
	{-9, "Max image height is exceed"},
	{-10, "Only 8, 24 and 32bit images are supported"},
	{-11, "Incorrect palette size"},
	{-12, "Not enough memory to allocate buffer"},
	{-13, "KPLIB failed to process the image"},
	{-999, "Unknown common image error"},

};

NAMED_TYPE gPalErrorsCommon[] = {
	
	{-1, "Error opening file"},
	{-2, "Incorrect palette size"},
	{-3, "Is not a raw PAL file"},
	{-4, "Unsupported palette format"},
	{-999, "Unknown common palette error"},
};

int imgFile2TileFunc(IMG2TILEFUNC img2tileFunc, char* filepath, int nTile)
{
	unsigned char* pData; int hFile, len, retn = -1;
	if ((hFile = open(filepath, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) < 0) return retn;
	else if ((len = filelength(hFile)) <= 0)
	{
		close(hFile);
		return retn;
	}

	pData = (unsigned char*)Resource::Alloc(len); read(hFile, pData, len); close(hFile);
	retn = img2tileFunc(pData, len, nTile);
	Resource::Free(pData);
	return retn;
}

int imgCheckResolution(int wh, int hg)
{
	int imgLen = wh*hg;
	if (wh > kMaxImageWidth) 		return -8; 			// max width exceed
	else if (hg > kMaxImageHeight) 	return -9; 			// max height exceed
	else if (imgLen <= 0) 			return -2; 			// ???
	else return 0;
}

int tile2tga(char* img, int nTile) {

	if (!tileLoadTile(nTile))
		return -2;
	
	short nTile2;
	int hFile, i = 0, j = 0, k = 0;
	int wh = tilesizx[nTile], hg = tilesizy[nTile], imgLen = wh*hg;
	if (!tileAllocSysTile(&nTile2, wh, hg)) return -3;
	else if ((hFile = open(img, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE)) < 0)
		return -1;
	
	BYTE* tga; TGA_HEADER header;
	memset(&header, 0, sizeof(header));
	memset(buffer, 0, sizeof(buffer));
	
	PICANM* pnm =& panm[nTile];
	sprintf(buffer, "%s%d:\r\n", 			kArtedSig, kArtedVer);
	sprintf(buffer2, "TileNum=%d\r\n", 		nTile);					strcat(buffer, buffer2);
	sprintf(buffer2, "Surface=%d\r\n", 		surfType[nTile]);		strcat(buffer, buffer2);
	sprintf(buffer2, "Shade=%d\r\n", 		tileShade[nTile]);		strcat(buffer, buffer2);
	sprintf(buffer2, "RFFVoxel=%d\r\n", 	voxelIndex[nTile]);		strcat(buffer, buffer2);
	
	sprintf(buffer2, "Picanm=%d,%d,%d,%d", pnm->xcenter, pnm->ycenter, pnm->type, pnm->frames, pnm->speed, viewType[nTile]);
	strcat(buffer, buffer2);
	
	header.idLength 	= (char)ClipHigh(strlen(buffer), 255);
	header.imageType 	= 2;
	header.imageWidth 	= (uint16)wh;
	header.imageHeight 	= (uint16)hg;
	header.pixelDepth 	= 32;
	header.xOrigin		= (uint16)pnm->xcenter;
	header.yOrigin		= (uint16)pnm->ycenter;
	header.imageDesc	|= 8;
	
	write(hFile, &header, sizeof(header));
	write(hFile, buffer, header.idLength);

	memcpy((void*)waloff[nTile2], (void*)waloff[nTile], imgLen);
	artedRotateTile(nTile2);
	artedFlipTileX(nTile2);
	artedFlipTileY(nTile2);
	
	BYTE* pTile = (BYTE*)waloff[nTile2];
	
	// must allocate to save faster :(
	tga = (BYTE*)Resource::Alloc(imgLen<<2);
	
	// TGA have different byte ordering
	while(i < imgLen)
	{
		j = pTile[i++];
		tga[k++] = gamepal[j].b;
		tga[k++] = gamepal[j].g;
		tga[k++] = gamepal[j].r;
		tga[k++] = (j == 255) ? 0 : 255;
	}
	
	write(hFile, tga, imgLen<<2);
	tilePurgeTile(nTile2, TRUE);
	Resource::Free(tga);
	close(hFile);
	return 0;
}

int tile2qbm(char* img, int nTile) {

	if (!tileLoadTile(nTile))
		return -2;
	
	short nTile2;
	int hFile, i = 0, wh = tilesizx[nTile], hg = tilesizy[nTile], imgLen = wh*hg;
	if (!tileAllocSysTile(&nTile2, wh, hg)) return -3;
	else if ((hFile = open(img, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE)) < 0)
		return -1;
	
	QBM_HEADER header;
	memset(&header, 0, sizeof(header));

	header.type		= kQBitmapTRAW;
	header.tcolor	= 255;
	header.width	= wh;
	header.height	= hg;
	header.bpl		= wh;
	header.xofs		= panm[nTile].xcenter;
	header.yofs		= panm[nTile].ycenter;
	
	write(hFile, &header, sizeof(header));
	memcpy((void*)waloff[nTile2], (void*)waloff[nTile], imgLen);
	artedRotateTile(nTile2); artedFlipTileY(nTile2);
	
	BYTE* pTile = (BYTE*)waloff[nTile2];

	write(hFile, pTile, imgLen);
	tilePurgeTile(nTile2, TRUE);
	close(hFile);
	return 0;
}

int tile2pcx(char* img, int nTile) {

	if (!tileLoadTile(nTile))
		return -2;
	
	short nTile2;
	int hFile, i = 0, wh = tilesizx[nTile], hg = tilesizy[nTile], imgLen = wh*hg;
	if (!tileAllocSysTile(&nTile2, wh, hg)) return -3;
	else if ((hFile = open(img, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE)) < 0)
		return -1;
	
	PCX_HEADER header;
	memset(&header, 0, sizeof(header));

	header.manufacturer = 0xA;
	header.version		= 0x5;
	header.bitsPerPixel	= 8;
	header.x1			= wh-1;
	header.y1			= hg-1;
	header.xDPI			= wh;
	header.yDPI			= hg;
	header.planes		= 1;
	header.bytesPerLine	= wh; // must be even?
	
	write(hFile, &header, sizeof(header));
	memcpy((void*)waloff[nTile2], (void*)waloff[nTile], imgLen);
	artedRotateTile(nTile2); artedFlipTileY(nTile2);
	BYTE* pTile = (BYTE*)waloff[nTile2];
		
	write(hFile, pTile, imgLen);
	i = 0xC; write(hFile, &i, 1);
	write(hFile, gamepal, sizeof(PALETTE));

	tilePurgeTile(nTile2, TRUE);
	close(hFile);
	return 0;
}

int tga2tile(unsigned char* pBuf, int bufLen, int nTile) {

#ifdef USE_KPLIB
		return kplibImg2Tile(pBuf, bufLen, nTile);
#else
	char sign[32];
	uint8_t pixel[4], color;
	int i = 0, j, k, wh, hg, imgLen, dataLen, packLen, ps = 4;
	BYTE* tga = NULL, *data = NULL, *unpk = NULL;
	TGA_HEADER header; RGB rgb, prevRgb;
	BOOL RLE = FALSE;
	PALETTE pal;

	memset(sign,  0, sizeof(sign));
	memset(pixel, 0, sizeof(pixel));

	IOBuffer io = IOBuffer(bufLen, pBuf);
	
	io.read(&header, sizeof(header));					// read header	
	io.seek(header.idLength, SEEK_CUR);					// skip imageID info 	
	wh = header.imageWidth;								// get width
	hg = header.imageHeight;							// get height
	header.mapLength*=((header.mapWidth > 16) ? 3 : 2); // palette length in bytes
	imgLen  = wh*hg;									// total length in bytes
	
	if (!header.imageType)		return -2;				// no image present
	#ifndef I2TDEBUG
	if ((i = imgCheckResolution(wh, hg)) != 0)
		return i;
	#endif
	

	switch (header.pixelDepth) {
		case 8:
		case 24:
		case 32:
			switch (header.imageType) {
				case 9: RLE = TRUE;
				case 1:
					ps = 1;
					header.imageType = 1;
					if (!header.mapType) return -7; // color mapped images must contain palette
					break;
				case 10: RLE = TRUE;
				case 2:
					header.imageType = 2;
					if (header.pixelDepth < 32) ps = 3;
					break;
				default:
					return -5;
			}
			break;
		default:
			return -10;
	}
		
	if ((i = header.mapLength) > 0)
	{
		if (i != sizeof(pal))
		{
			// i don't know how to scale 512 bytes palette to 768 bytes, so end with error :(
			if (i < sizeof(pal) || header.imageType == 1) return -11;
			else
			{
				// skip non 256 color palette for non-color mapped images
				// since it's probably used by software that created the file
				io.seek(i, SEEK_CUR);
			}
		}
		else
		{
			io.read(pal, sizeof(pal));							// read the 768 bytes palette
			palFixTransparentColor(pal);						// lookup in palette db to fix transparent color
		}
	}

	i = io.tell();												// save img-start offset
	k = strlen(kTargaV20Sig) + 1;								// v2.0 signature + NULL byte
	io.seek(-k, SEEK_END);										// must go to the end of file													
	io.read(sign, k);											// read signature bytes
	j = io.tell();												// save pre img-end offset
		
	if (strcmp(sign, kTargaV20Sig) == 0)
	{
		j = io.seek(-(k + ((sizeof(int32_t)<<1))), SEEK_END);	// go to the top of v2.0 offsets map
		io.read(&k, sizeof(int32_t));							// read "Extension Area" offset
		if (k != 0)												// is "Extension Area" present?
			j = io.seek(j, SEEK_SET);							// img-end is the "Extension Area" offset now
	}

	packLen = j - i;											// compressed length in bytes
	dataLen = ClipLow(imgLen*ps, packLen);						// uncompressed length in bytes
	data = (BYTE*)Resource::Alloc(packLen);	 unpk = data;		// allocate packed data array
	io.seek(i, SEEK_SET);										// back to the img-start offset
	io.read(data, packLen);										// read all the data
	
	dassert(imgLen*ps <= dataLen);
	i = j = k = 0;

	if (RLE)													// decode image here...
	{
		unpk = (BYTE*)Resource::Alloc(dataLen);					// allocate unpacked data array
		while(i < packLen)
		{
			j = data[i++];
			if (j & 0x80)
			{
				j = (j & 0x7F) + 1;
				memcpy(pixel, &data[i], ps); i+=ps;
				while(j-- > 0)
				{
					memcpy(&unpk[k], pixel, ps);
					k+=ps;
				}
			}
			else
			{
				++j;
				while(j-- > 0)
				{
					memcpy(&unpk[k], &data[i], ps);
					k+=ps, i+=ps;
				}
			}
		}
		
		Resource::Free(data);									// don't need it anymore
	}

	i = j = k = 0;
	tga = (BYTE*)Resource::Alloc(imgLen);						// allocate actual image
	while (j < imgLen)											// convert colors here...
	{
		memcpy(pixel, &unpk[i], ps);
		if (header.imageType != 2 || header.pixelDepth != 32 || pixel[3] != 0)
		{
			switch (header.imageType) {
				case 1:
					k = pixel[0];
					rgb.r = pal[k].b;
					rgb.g = pal[k].g;
					rgb.b = pal[k].r;
					break;
				case 2:
					rgb.r = pixel[2];
					rgb.g = pixel[1];
					rgb.b = pixel[0];
					break;
			}
			
			if (memcmp(&prevRgb, &rgb, sizeof(RGB)) != 0)
				color = countBestColor(gamepal, rgb.r, rgb.g, rgb.b);
			
			tga[j] = color, prevRgb = rgb;
		}
		else
		{
			tga[j] = 255;
		}
		
		j++, i+=ps;
	}

	Resource::Free(unpk);								// don't need it anymore
	if ((i = rawImg2Tile(tga, nTile, wh, hg, 1)) >= 0)	// image description
	{
		switch ((j = (header.imageDesc & 0x30) >> 4)) {
			case 1: // bottom right
			case 0:	// bottom left
				artedFlipTileY(nTile, FALSE);
				if (j != 1)
					break;
			case 3: // top right
				artedFlipTileX(nTile, FALSE);
				break;
		}
	}
	
	Resource::Free(tga);
	return i;
#endif

}

int pcx2tile(unsigned char* pBuf, int bufLen, int nTile) {

	// some images converts wrongly or cannot be at all with KPLIB
	
	int r, g, b, skip;
	int i = 0, j, k, f, wh, hg, imgLen, dataLen, packLen, bits, bpl;
	BYTE* pcx = NULL, *data = NULL, *unpk = NULL;
	PALETTE pal; BYTE hasPal; PCX_HEADER header;

	IOBuffer io = IOBuffer(bufLen, pBuf);
	
	io.read(&header, sizeof(PCX_HEADER));				// read header
	wh = header.x1 - header.x0 + 1;						// get width
	hg = header.y1 - header.y0 + 1;						// get height
	imgLen = wh * hg;									// total length in bytes
	
	#ifndef I2TDEBUG
	if ((i = imgCheckResolution(wh, hg)) != 0)
		return i;
	#endif
	
	bpl  = header.bytesPerLine;							// bytes per one scan line
	skip = bpl - wh;									// how many pad bytes added to each scan line
	bits = header.planes*header.bitsPerPixel;			// bits per PLANES

	k = io.tell();										// save current offset
	switch(bits) {
		default:
			return -5;
		case 24:
			j = io.seek(0, SEEK_END);					// image data should end here
			break;
		case 8:
			j = io.seek(-769, SEEK_END);				// must go to to the palette offset
			io.read(&hasPal, 1);						// read palette sign
			if (hasPal != 0x0C) return -7;				// pal sign found
			io.read(pal, sizeof(pal));					// read the palette
			break;
	}

	io.seek(k, SEEK_SET);								// return back

	packLen = j - k;									// count packed size (RLE can make file larger!)
	dataLen = ClipLow((bpl*header.planes)*hg, packLen);	// count unpacked data size
	data = (BYTE*)Resource::Alloc(dataLen);				// allocate image data array
	io.read(data, packLen);								// read all the data

	i = j = f = 0;
	if (header.encoding == 1)							// decode the image
	{
		unpk = (BYTE*)Resource::Alloc(dataLen);			// allocate raw image array
		while (f < packLen)
		{
			j = data[f++];								// read byte
			if ((j & 0xC0) == 0xC0)						// RLE compressed
			{
				k = j & 0x3F;							// how many times we should repeat
				j = data[f++];							// which value?
				memset(&unpk[i], j, k);					// do repeat
				i+=k;
			}
			else										// no compression
			{
				unpk[i++] = (BYTE)j;
			}
		}
		memcpy(data, unpk, dataLen);
		Resource::Free(unpk);
	}

	pcx = (BYTE*)Resource::Alloc(imgLen);				// allocate the actual image
	i = j = f = 0;

	switch (bits) {
		case 8:
			while(i < imgLen)
			{
				pcx[i++] = data[f++];
				if (++j == wh)
					j = 0, f+=skip;
			}
			palFixTransparentColor(pal);				// lookup in palette db to fix transparent color
			remapColors((intptr_t)pcx, imgLen, pal); 	// remap colors of the image to the kPal0
			break;
		case 24:										// colors stored as arrays (planes)
			r = 0, g = r + bpl, b = g + bpl;
			while(i < imgLen)
			{
				pcx[i++] = countBestColor(gamepal, data[r++], data[g++], data[b++]);
				if (++j == wh)
					j = 0, r = b + skip, g = r + bpl, b = g + bpl;
			}
			break;
	}

	Resource::Free(data);
	i = rawImg2Tile(pcx, nTile, wh, hg);
	Resource::Free(pcx);
	return i;
}

int qbm2tile(unsigned char* pBuf, int bufLen, int nTile) {

	int wh, hg, imgLen, retn = 0;
	BYTE* qbm = NULL; QBM_HEADER header;
	IOBuffer io = IOBuffer(bufLen, pBuf);
	
	io.read(&header, sizeof(header));
	wh = header.width; hg = header.height; imgLen = wh*hg;
	#ifndef I2TDEBUG
	if ((retn = imgCheckResolution(wh, hg)) != 0)
		return retn;
	#endif
	
	switch (header.type) {
		case kQBitmapRLE:
			return -6;
		case kQBitmapRAW:
		case kQBitmapTRAW:
			break;
		default:
			return -5; // unknown
	}
	
	qbm = (BYTE*)Resource::Alloc(imgLen); io.read(qbm, imgLen);
	if ((retn = rawImg2Tile(qbm, nTile, wh, hg)) >= 0)
	{
		if (header.type == kQBitmapTRAW && header.tcolor != 255)
			artedReplaceColor(nTile, (char)header.tcolor, 255);
	}
	
	Resource::Free(qbm);
	return retn;

}


int cel2tile(unsigned char* pBuf, int bufLen, int nTile) {
	
#ifdef USE_KPLIB
		return kplibImg2Tile(pBuf, bufLen, nTile);
#else
	int i, wh, hg, imgLen, retn = 0;
	BYTE* cel = NULL; CEL_HEADER header; PALETTE pal;
	IOBuffer io = IOBuffer(bufLen, pBuf);
	
	io.read(&header, sizeof(header));
	wh = header.width; hg = header.height; imgLen = wh*hg;
	#ifndef I2TDEBUG
	if ((retn = imgCheckResolution(wh, hg)) != 0)
		return retn;
	#endif
		
	if (header.magic != 0x9119 || header.bpp != 8) return -5;
	else if (header.compression != 0)
		return -6;
	
	io.read(pal, sizeof(pal));
	if (io.nRemain != header.bytes)
		return -4;

	// lookup in palette db to fix transparent color
	palFixTransparentColor(pal);
	
	// palette shift
	palShift(pal, 2);

	cel = (BYTE*)Resource::Alloc(imgLen); io.read(cel, imgLen);
	if ((retn = rawImg2Tile(cel, nTile, wh, hg)) >= 0)
		remapColors(waloff[nTile], imgLen, pal);
	
	Resource::Free(cel);
	return retn;
#endif
}

int png2tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int retn;
	#ifdef USE_KPLIB
	retn = kplibImg2Tile(pBuf, bufLen, nTile);
	#else
	retn = -5;
	#endif
	return retn;
}


int bmp2tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int retn;
	#ifdef USE_KPLIB
	retn = kplibImg2Tile(pBuf, bufLen, nTile);
	#else
	retn = -5;
	#endif
	return retn;
}

int dds2tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int retn;
	#ifdef USE_KPLIB
	retn = kplibImg2Tile(pBuf, bufLen, nTile);
	#else
	retn = -5;
	#endif
	return retn;
}

int jpg2tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int retn;
	#ifdef USE_KPLIB
	retn = kplibImg2Tile(pBuf, bufLen, nTile);
	#else
	retn = -5;
	#endif
	return retn;
}

int gif2tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int retn;
	#ifdef USE_KPLIB
	retn = kplibImg2Tile(pBuf, bufLen, nTile);
	#else
	retn = -5;
	#endif
	return retn;
}

#ifdef USE_KPLIB
int kplibImg2Tile(unsigned char* pBuf, int bufLen, int nTile)
{
	int wh, hg;
	int imgLen, retn = 0; BYTE* pixels;
	
	kpgetdim(pBuf, bufLen, &wh, &hg); imgLen = wh*hg;
	if (!wh || !hg)
		return -13;
	
	#ifndef I2TDEBUG
	if ((retn = imgCheckResolution(wh, hg)) != 0)
		return retn;
	#endif
	
	imgLen = hg*(wh<<2);
	if ((pixels = (BYTE*)malloc(imgLen)) == NULL) return -12;
	else if (kprender(pBuf, bufLen, (void*)pixels, wh<<2, wh, hg, 0, 0) < 0) retn = -13;
	else if (convertPixels(pixels, imgLen, 4) >= 0 && rawImg2Tile(pixels, nTile, wh, hg) > 0) retn = 0;
	free(pixels);
	
	return retn;
}
#endif

int rawImg2Tile(BYTE* image, int nTile, int wh, int hg, int align) {

	BYTE* pTile;
	int i = 0, j = 0, k, f, len = wh*hg;
	if (len <= 0)
		return -2;
	
	artedEraseTileImage(nTile);
	if ((pTile = tileAllocTile(nTile, wh, hg, 0, 0)) == NULL)
		return -3;

	switch(align) {
		default:							// as is
			memcpy(pTile, image, len);
			break;
		case 1: 							// top-left -> right-bottom
			while (i < len)
			{
				pTile[i] = image[j]; 		// COLUMN of the image (ROW of the tile)
				k = i, f = j; 				// read ROWS of the image vertically from the top to the bottom
				while ((f += wh) < len)
				{
					if (++k >= len)
						return i;


					pTile[k] = image[f];
				}

				i+=hg, j++;
			}
			break;
	}

	return 0;
}

int convertPixels(BYTE* pixels, int bufLen, int pxsize, char rgbOrder)
{
	BYTE p[4], o[3];
	int i = 0, j = 0;
	BOOL trans;
	
	switch (rgbOrder)
	{
		case 1 :  o[0] = 0, o[1] = 1, o[2] = 2;
		default:  o[0] = 2, o[1] = 1, o[2] = 0;
	}

	switch (pxsize) {
		case 4:
			trans = TRUE;
			// no break
		case 3:
			while(i < bufLen)
			{
				memcpy(p, &pixels[i], pxsize);
				pixels[j++] = (trans && p[3]) ? countBestColor(gamepal, p[o[0]], p[o[1]], p[o[2]]) : 255;
				i+=pxsize;
			}
			return 0;
	}
	
	return -1;
}

int pluLoad(char* fname, PALETTE out, int nPlu, int nShade) {
	
	dassert(out != NULL);
	
	PALETTE pal;
	BYTE tmp[256]; short numlooks = 0;
	int i = 0, j, flen, hFile, palsiz = sizeof(PALETTE);
	if ((hFile = open(fname, O_RDONLY|O_BINARY, S_IWRITE)) < 0) return -1;
	else if ((flen = lseek(hFile, 0, SEEK_END)) < palsiz)
	{
		close(hFile);
		return -2;
	}
	
	lseek(hFile, 0, SEEK_SET);
	switch (palGetType(fname)) {
		case kPaletteDAT:
			read(hFile, out, palsiz);
			while (i < 256) { out[i].r<<=2, out[i].g<<=2, out[i].b<<=2; i++; }
			if ((flen-kPluAreaLenNew) % 256 == 0) read(hFile, &numlooks, 2);
			else if ((i = (flen-kPluAreaLenOld)) % 256 == 0) numlooks = i>>8;
			else return -3;
			
			if ((i = (numlooks*256)*nPlu) >= flen)
				return -4;

			lseek(hFile, i, SEEK_CUR); read(hFile, tmp, 256); i = 0;
			while (i < 256) { out[i] = out[tmp[i]]; i++; }
			break;
	}
	close(hFile);
	return 0;
}


int palLoad(char* fname, PALETTE out) {
	
	dassert(out != NULL);
	
	BYTE tmp[256];
	int i = 0, j = -1, retn = 0, flen, hFile;
	int palsiz = sizeof(gamepal), rgbsiz = sizeof(gamepal[0]);
	PALETTE pal;
	
	if ((hFile = open(fname, O_RDONLY|O_BINARY, S_IWRITE)) < 0) return -1;
	else if ((flen = lseek(hFile, 0, SEEK_END)) < palsiz)
	{
		close(hFile);
		return -2;
	}
	
	lseek(hFile, 0, SEEK_SET);
	switch (palGetType(fname)) {
		default:
			retn = -4;
			break;
		case kPaletteDAT:								
			if (((flen - 65536 - 2 - 768) % 256 == 0) || ((flen - 32640 - 768) % 256 == 0))
			{
				// standard BUILD palette
				read(hFile, out, palsiz);
				palShift(out);
				break;
			}
			retn = -2;
			break;
		case kPalettePAL:
		case kPalettePaintShop:
		case kPaletteMicrosoft:
			for (i = 0; i < LENGTH(gPALSigns); i++)
			{
				lseek(hFile, 0, SEEK_SET);
				memset(tmp, 0, sizeof(tmp));
				read(hFile, tmp, strlen(gPALSigns[i].name));
				if (strcmp((char*)tmp, gPALSigns[i].name) == 0)
				{
					j = gPALSigns[i].id;
					break;
				}
			}

			i = 0;
			switch ( j ) {
				case kPalettePaintShop:
					j = 0; memset(tmp, 0, sizeof(tmp));
					lseek(hFile, 2, SEEK_CUR); read(hFile, tmp, strlen(kPaintShopPalSig));
					if (strcmp((char*)tmp, kPaintShopPalSig) == 0)	// second sign check
					{
						memset(tmp, 0, sizeof(tmp)); lseek(hFile, 7, SEEK_CUR);
						while (i < 256)
						{
							if (j > 12 || read(hFile, &tmp[j], 1) != 1) break;
							else if (tmp[j] != '\n') j++;
							else
							{
								tmp[j - 1] = 0, j = 0;
								pal[i].r = (uint8_t)enumStrGetInt(0, (char*)tmp,  ' ', 255);
								pal[i].g = (uint8_t)enumStrGetInt(1, NULL, 		  ' ', 255);
								pal[i].b = (uint8_t)enumStrGetInt(2, NULL,        ' ', 255);
								i++;
							}
						}
						
						if (tell(hFile) == lseek(hFile, 0, SEEK_END) && i == 256)
						{
							memcpy(out, pal, palsiz);
							break;
						}
					}
					retn = -4;
					break;
				case kPaletteMicrosoft: // looks like a Windows palette
					lseek(hFile, sizeof(int32_t), SEEK_CUR);
					read(hFile, tmp, strlen(kRiffPalSig));
					if (strcmp((char*)tmp, kRiffPalSig) == 0)	// second sign check
					{
						lseek(hFile, sizeof(int32_t)<<1, SEEK_CUR);
						while (i < 256 && read(hFile, &pal[i++], rgbsiz) == rgbsiz) lseek(hFile, 1, SEEK_CUR);
						if (tell(hFile) == lseek(hFile, 0, SEEK_END) && i == 256)
						{
							memcpy(out, pal, palsiz);
							break;
						}
					}
					// no break
				default:
					if (flen == palsiz)
					{
						lseek(hFile, 0, SEEK_SET);
						read(hFile, out, palsiz); // raw 768 bytes RGB info
					}	
					else
					{
						retn = -4;

					}
					break;
			}
			break;
		// blood's palookups
		case kPalettePLU:
			if (flen / 256 == 64)
			{
				memcpy(out, gamepal, palsiz);					// copy normal palette first
				read(hFile, tmp, LENGTH(tmp)); tmp[255] = 255;	// transparent color fix
				while (i < 256) { out[i] = out[tmp[i]]; i++; }	// replace color indexes
			}
			else
			{
				retn = -2;
			}
			break;
	}
	
	close(hFile);
	return retn;
}

void palShift(PALETTE pal, int by)
{
	int i = 0;
	while(i < 256)
	{
		pal[i].r = (unsigned char)ClipHigh(pal[i].r << by, 255);
		pal[i].g = (unsigned char)ClipHigh(pal[i].g << by, 255);
		pal[i].b = (unsigned char)ClipHigh(pal[i].b << by, 255);
		i++;
	}
}

BOOL palFixTransparentColor(PALETTE imgPal) {
	
	if (!fileExists(kPalDBIni))
		return FALSE;

	IniFile* palDb = new IniFile(kPalDBIni);
	ININODE* prev = NULL; char* key = NULL, *value = NULL;
	int i, j, tokens = LENGTH(palDBTokens);
	PALETTE palbuf; BOOL retn = FALSE;

	while (palDb->GetNextString(NULL, &key, &value, &prev))
	{
		if (value == NULL)
			continue;

		for (j = 0; j < tokens; j++)
		{
			if (stricmp(key, palDBTokens[j].name) == 0)
				break;
		}

		if (j < tokens)
		{
			i = -1;
			switch(palDBTokens[j].id) {
				default:
					continue;
				case kTokenPAL: // compare entire palette and remap 255's to transparent
					memset(palbuf, 0, sizeof(palbuf));
					if (palLoad(value, palbuf) == 0)
					{
						if (memcmp(imgPal, palbuf, sizeof(palbuf)) != 0) continue;
						i = 255;
						break;
					}
					continue;
				case kTokenRGB: // RGB value that must be remapped to the transparent RGB
					if ((i = enumStrGetInt(0, value, ',', -1)) < 0 || i >= 256) continue;
					palbuf[255].r = (char)enumStrGetInt(1, NULL, ',', gamepal[255].r);
					palbuf[255].g = (char)enumStrGetInt(2, NULL, ',', gamepal[255].g);
					palbuf[255].b = (char)enumStrGetInt(3, NULL, ',', gamepal[255].b);
					break;
			}

			if (memcmp(&imgPal[i], &palbuf[255], sizeof(RGB)) == 0)
			{
				memcpy(&imgPal[i], &gamepal[255], sizeof(RGB)); // found
				retn = TRUE;
				break;
			}
		}
	}

	delete palDb;
	return retn;

}

void remapColors(intptr_t image, int len, PALETTE srcPal)
{
	// build remap table
	BYTE RemapColor[256];
	for (int i = 0; i < 256; i++)
		RemapColor[i] = countBestColor(gamepal, srcPal[i].r, srcPal[i].g, srcPal[i].b);

	// remap colors
	bufRemap((BYTE*)image, len, RemapColor);
}

void bufRemap(BYTE* buf, int len, BYTE* table)
{
	int i = 0; while (i < len) buf[i] = table[buf[i++]];
}


int imgGetType(char* str) {

	return getTypeByExt(str, gSuppImages, LENGTH(gSuppImages));
}

int palGetType(char* str) {
	
	return getTypeByExt(str, gSuppPalettes, LENGTH(gSuppPalettes));
	
}

IMG2TILEFUNC imgGetConvFunc(int nImgType)
{
	return (IMG2TILEFUNC)getFuncPtr(gImgFuncs, LENGTH(gImgFuncs), nImgType);
}


BYTE countBestColor(PALETTE in, int r, int g, int b, int wR, int wG, int wB)
{
	int i, dr, dg, db;
	int dist, matchDist = 0x7FFFFFFF, match;
	
	for (i = 0; i < 256; i++)
	{
		dist = 0;
		dg = (int)in[i].g - g;
		dist += wG * dg * dg;
		if (dist >= matchDist)
			continue;

		dr = (int)in[i].r - r;
		dist += wR * dr * dr;
		if (dist >= matchDist)
			continue;

		db = (int)in[i].b - b;
		dist += wB * db * db;
		if (dist >= matchDist)
			continue;

		matchDist = dist;
		match = i;

		if (dist == 0)
			break;
	}

	return (BYTE)match;
}

BYTE LookupClosestColor(BYTE* inverseMap, int r, int g, int b )
{
	r >>= 2;
	g >>= 2;
	b >>= 2;
	return inverseMap[(((r << 6) | g) << 6) | b];
}

BOOL BuildPLU(BYTE* out, PALETTE pal, int grayLevel) {
	
	dassert(out != NULL);
	
	RESHANDLE hMap;
	if ((hMap = gGuiRes.Lookup((uint32_t)0, "DAT")) == NULL)
		return FALSE;
	
	int i, j, r, g, b, n;
	BYTE *inverseMap = (BYTE*)gGuiRes.Load(hMap);
	BYTE *p = out;

	for ( i = 0; i < 64; i++ )
	{
		n = scale(i, 0xE000, 64);
		for ( j = 0; j < 256; j++ )
		{
			r = interpolate(pal[j].r, grayLevel, n);
			g = interpolate(pal[j].g, grayLevel, n);
			b = interpolate(pal[j].b, grayLevel, n);
			*p++ = LookupClosestColor(inverseMap, r, g, b);
		}
	}
	
	return TRUE;
}

BOOL BuildPLU(char *filename, PALETTE pal, int grayLevel)
{
	BYTE *table = (BYTE*)Resource::Alloc(256 * 64);
	BuildPLU(table, pal, grayLevel);
	
	ChangeExtension(filename, ".plu");
	FileSave(filename, table, 256 * 64);
	Resource::Free(table);
	
	return TRUE;
}
