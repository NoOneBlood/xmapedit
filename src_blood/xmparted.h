/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Implementation of ART files editor (ARTEDIT).
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

#ifndef __XMPARTED
#define __XMPARTED
#include "common_game.h"
#include "xmptools.h"
#include "gfx.h"
#include "seq.h"
#include "screen.h"
#include "tile.h"

#define kMaxArtFiles 256
#define kArtBlockMapSign "<-XrT\x1A"
#define kTPF 512
#define kTPFS kTPF - 1

#define kArtedSig "XMPARTED"
#define kArtedVer 1

enum {
kDirtyNone 				= 0x00,
kDirtyArt 				= 0x01,
kDirtyDat 				= 0x02,
kDirtyPicanm			= 0x04,
kDirtyAll				= 0xFF,
};

enum {
kArtEdModeNone			= 0,
kArtEdModeBatch			= 1,
kArtEdModeTile2D		= 2,
kArtEdModeMax			   ,
};

enum {
kArtSaveFlagNone		= 0x00,
kArtSaveFlagTileInfo	= 0x01,
};

enum {
kArtXBlockTileMixed		= 64,
kArtXBlockGlobalPalette	= 128,
};

enum {
kImage					= 0x01,
kAnimation				= 0x02,
kShade					= 0x04,
kSurface				= 0x08,
kVoxelID				= 0x10,
kView					= 0x20,
kOffsets				= 0x40,		
kAll					= 0x80,
};

enum {
kArtSignBAFED 			= 0,	// replaces artversion and 1 byte of numtiles
kArtSignBUILDART 		= 1,	// offsets standard header to sign len bytes
};

#pragma pack(push,1)
struct XRTBLK_HEAD {

	BYTE	 blktype;
	BYTE    headsiz;
	ushort   datasiz;

};

struct XRTBLKa_DATA {
	
	ushort number;
	schar  shade;
	BYTE  surface;
	short voxelID;
	
};

struct XRTBLKb_DATA {
	
	PALETTE artPal;
	
};

struct ARTFILE {

	unsigned int start   	: 16;
	unsigned int end     	: 16;
	unsigned int dirty		: 1;
	char path[_MAX_PATH];
	
};

struct ARTEDIT {
	
	POINT origin;
	BYTE asksave;
	unsigned int nTile		: 16;
	unsigned int nVTile		: 16;
	unsigned int mode 		: 4;
	unsigned int modeMin	: 4;
	unsigned int modeMax	: 4;
	unsigned int standalone	: 1;
};
#pragma pack(pop)


extern ARTEDIT gArtEd;
extern ARTFILE gArtFiles[kMaxArtFiles];
extern int nArtFiles;

extern STATUS_BIT1 gHgltTiles[kMaxTiles];
extern BOOL gDirtTiles[kMaxTiles];

extern char* gArtEdModeNames[3];
extern NAMED_TYPE paintMenu[2];
extern NAMED_TYPE artedSaveMenu[2];
extern NAMED_TYPE importMenu[3];
typedef void HTILEFUNC1( int nTile, int value);

void artedInit();
void artedUninit();
void artedStart(char* filename, BOOL standalone);
int artedDrawTile(int nTile, int nOctant, int nShade);
BOOL artedProcessEditKeys(BYTE key, BYTE ctrl, BYTE alt, BYTE shift);

void artedArtDirty(int nTile, char what);
void artedSaveChanges();
void artedRevertChanges();
void artedSaveArtFiles();
void artedSaveDatFiles();

int artedDlgSaveSelection(int nTile);
int artedDlgViewTypeSet(int nTile);
int artedDlgSelectVoxel(int nTile);
int artedDlgImportImage(int nTile);
int artedDlgImportArt(int nTile);
int artedDlgImportFLOORS(int nTile);
int artedDlgPaint(int nTile);
int artedDlgCopyTiles(int nTile, int total);
int artedDlgSelectTilePart(char* title);
BOOL artedDlgReplaceColor(int nTile);
int artedDlgSelPal(char* title, char* path, PALETTE out);

BOOL artedPaint(int nTile, PALETTE pal);
int artedReplaceColor(int nTile, BYTE colorA, BYTE colorB = 255);
void artedSurfaceSet(int nTile, int value);
void artedShadeSet(int nTile, int value);
void arttedShadeIterate(int nTile, int value);
void artedAnimTypeSet(int nTile, int value);
void artedEraseTileImage(int nTile, int value = 0);
void artedEraseTileInfo(int nTile, int value = 0);
void artedEraseTileFull(int nTile, int value = 0);
void artedRevertTile(int nTile, int what = 0x0);
void artedRotateTile(int nTile, int ofs = 0);
void artedFlipTileY(int nTile,  int ofs = 0);
void artedFlipTileX(int nTile,  int ofs = 0);
void artedCopyTile(int nTileSrc, int nTileDest, int what = 0x0);
void artedPurgeUnchanged();
int artedArray2Art(char* fname, short* array, int len, int startID = 0, char flags = 0x0, PALETTE pal = NULL);

void artedViewZoomReset();
void artedCleanUp();
void artedCleanUpTile(int nTile);


char* artedGetFile(int nTile);
int artedBatchProc(HTILEFUNC1 tileFunc, int data = 0, char dirty = 0);
int hgltTileCallFunc(HTILEFUNC1 tileFunc, int data = 0, char dirty = 0);
int hgltTileCount();
void hgltTileReset();
void hgltTileAdd(int nTile);
void hgltTileRem(int nTile);
void hgltTileToggle(int nTile);
void hgltTileSelectRange(int nTile);
BOOL tileInHglt(int nTile);
BOOL isSysTile(int nTile);
BOOL canEdit();

int readArtHead(int hFile, int* tstart, int* tend, int* sixofs, int* siyofs, int* pnmofs, int* datofs);
int readArtHead(char* file, int* tstart, int* tend, int* sixofs, int* siyofs, int* pnmofs, int* datofs);
int readTileArt(int nTile, int hFile, short* six = NULL, short* siy = NULL, BYTE** image = NULL, int32_t* imgofs = NULL, PICANM* pnm = NULL);
int readTileArt(int nTile, char* file, short* six = NULL, short* siy = NULL, BYTE** image = NULL, int32_t* imgofs = NULL, PICANM* pnm = NULL);
int readTileShade(int nTile, char* file, schar* out);
int readTileSurface(int nTile, char* file, BYTE* out);
int readTileVoxel(int nTile, char* file, short* out);
BOOL xartOffsets(int hFile, int* blocksCount, int* mapofs);
void artedLogMsg(char *__format, ...);
#endif
