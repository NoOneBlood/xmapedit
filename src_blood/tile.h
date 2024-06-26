/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Edited by NoOne and Nuke.YKT.
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

#ifndef __TILE_H
#define __TILE_H

#include "common_game.h"
#include "tilefav.h"

struct SYSTEM_TILES{
	unsigned int tileViewBg			: 17;
	unsigned int gameMirrorStart	: 17;
	unsigned int gameMirrorEnd		: 17;
	unsigned int angArrow			: 17;
	unsigned int wallHglt			: 17;
	unsigned int drawBuf			: 17;
	unsigned int icoNoTile			: 17;
	unsigned int icoXmp				: 17;
	unsigned int icoVer1			: 17;
	unsigned int icoVer2			: 17;
	unsigned int icoFolder1			: 17;
	unsigned int icoFile1			: 17;
	unsigned int icoDrive1			: 17;
	unsigned int icoDiskette		: 17;
	unsigned int icoPreview			: 17;
	unsigned int hudMask			: 17;
	STATUS_BIT1 busy[kMaxTiles];
	BOOL isBusy(int nTile) 	{ return (BOOL)busy[nTile].ok; }
	void add(int nTile) 	{ busy[nTile].ok = 1; }
	void addAll() 			{ memset(busy, 1, sizeof(busy)); }
	void rem(int nTile) 	{ busy[nTile].ok = 0; }
	void remAll() 			{ memset(busy, 0, sizeof(busy)); }
	void toggle(int nTile) 	{ busy[nTile].ok ^= 1; }
};


extern SYSTEM_TILES gSysTiles;

extern char *viewNames[8];
extern char *surfNames[15];
extern char *animNames[4];

extern short tileIndex[kMaxTiles];
extern short tileIndexCount;
extern short tileIndexCursor;
extern char tilePluIndex[kMaxTiles];
extern short voxelIndex[kMaxTiles];

extern BYTE surfType[kMaxTiles];
extern BYTE viewType[kMaxTiles];
extern schar tileShade[kMaxTiles];

int tileInitFromIni();




BYTE *tileLoadTile( int nTile );
void tilePreloadTile( int nTile, int objType = -1);
BYTE *tileAllocTile( int nTile, int sizeX, int sizeY, int offX = 0, int offY = 0 );
void tileFreeTile( int nTile );
void tilePurgeTile( int nTile, BOOL force = FALSE);
void tilePurgeAll(BOOL force = FALSE);


int tileBuildHistogram( int type );


enum {
kTilePickFlgAllowEmpty		= 0x01,
};

int tilePick(int nTile, int nDefault, int type, char* titleArg = NULL, char flagsArg = 0x0);
void tileDrawTileRect(Rect** pARect, int flags, int nTile, int nSize, int nPlu, int nShade, int nDrawFlags = 0x02);
void tileDrawTileRect(Rect* pARect, int flags, int nTile, int nSize, int nPlu, int nShade, int nDrawFlags = 0x02);
void tileDrawTile(int x, int y, short pic, int size, short plu, char flags, schar shade = 0);
void tileDrawGetSize(short pic, int size, int* width, int* height);
int tileGetPic(int nTile);

BYTE tileGetSurfType( int nHit );
void tileShowInfoWindow(int nTile);
short tileSearchFreeRange(int range);
short tileGetBlank();

BYTE tileGetMostUsedColor(int nTile, short noColor = 255);
int tileReplaceColor(int nTile, char colorA, char colorB = 255);
BOOL tileHasColor(int nTile, char color);
int tileGetUsedColors(int nTile, char colors[256]);
int tileCountColors(int nTile);
int tileExists(BYTE* image, int wh, int hg);

void tilePaint(int nTile, int nPlu, int nShade = 0, int nVis = 0);
BOOL tileAllocSysTile(short* dest, int w, int h);
void tileInitSystemTiles();
void tileUninitSystemTiles();
void CalcPicsiz( int nTile, int x, int y );
void qloadtile(short nTile);
void qloadvoxel( int nVoxel );
inline char isPow2X(int nTile) { return (pow2long[picsiz[nTile] & 15] == tilesizx[nTile]); }
inline char isPow2Y(int nTile) { return (pow2long[picsiz[nTile] >> 4] == tilesizy[nTile]); }
inline char isPow2(int nTile) { return (isPow2X(nTile) && isPow2Y(nTile)); }

#endif

