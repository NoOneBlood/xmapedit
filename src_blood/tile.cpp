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

#include "common_game.h"
#include "editor.h"
#include "tile.h"
#include "aadjust.h"
#include "xmpstub.h"
#include "gui.h"
#include "screen.h"
#include "fire.h"
#include "tilefav.h"
#include "inifile.h"
#include "edit3d.h"
#include "replace.h"
#include "osd.h"
#include "crc32.h"
#include "xmpror.h"
#include "img2tile.h"
#include "xmparted.h"
#include "xmpexplo.h"
#include "xmpconf.h"
#include "xmpmisc.h"
#include "xmptools.h"
#include "xmpevox.h"

#include "cache1d.h"
extern "C" {
#include "a.h"
}



SYSTEM_TILES gSysTiles;
CACHENODE tileNode[kMaxTiles];
int tileHist[kMaxTiles];

short tileIndex[kMaxTiles], voxelIndex[kMaxTiles], tileIndexCursor, tileIndexCount;
char tilePluIndex[kMaxTiles];
BYTE surfType[kMaxTiles], viewType[kMaxTiles];
schar tileShade[kMaxTiles];


char *viewNames[] = {

	"Single",
	"5 Full",
	"8 Full",
	"Bounce",
	"5 Half",
	"3 Flat",
	"Voxel",
	"Spin voxel",

};

char *viewNamesShort[] = {
	
	"Sigl",
	"5Ful",
	"8Ful",
	"Bnce",
	"5Hal",
	"8Hal",
	"Voxl",
	"Spin",
};

char *surfNames[] = {

	"None",
	"Stone",
	"Metal",
	"Wood",
	"Flesh",
	"Water",
	"Dirt",
	"Clay",
	"Snow",
	"Ice",
	"Leaves",
	"Cloth",
	"Plant",
	"Goo",
	"Lava",
};

char *animNames[] =
{
	"None",
	"Oscil",
	"Forward",
	"Backward",
};

void tileScrOptionsDlg() {
	
	CHECKBOX_LIST_P tileViewOpts[] = {

		{&gTileView.stretch, "&Stretch tiles."},
		{&gTileView.showTransColor, "Show &transparent color."},
		{&gTileView.showAnimation, "Play tile &animation."},
		{&gTileView.transBackground, "Translucent &background."},
		{&gTileView.showInfo, "Always show misc tile &info."},

	};
	
	createCheckboxList(tileViewOpts, LENGTH(tileViewOpts), "Tile screen options", TRUE);
	return;
	
}

void tileScrDrawStatus(int x1, int y1, int x2, int y2) {

	QFONT* pFont = qFonts[1];
	int width  = x2 - x1;
	int height = y2 - y1;
	int fh = ClipHigh(pFont->height, height);
	int tx1 = x1;
	int ty1 = (y1 + (fh >> 1)) - 1;
	int rmar = 1, lmar = 1;
	int len;

	gfxSetColor(gStdColor[28]);
	gfxFillBox(x1, y1, x1 + width, y1 + height);
	gfxSetColor(gStdColor[22]);
	gfxHLine(y1, x1, x1 + width);

	// left part
	///////////////////////
	sprintf(buffer, "Tile #%04d", gArtEd.nTile);
	len = gfxGetTextLen(strupr(buffer), pFont);
	lmar+=len+2;

	gfxSetColor(gStdColor[3]);
	gfxFillBox(tx1+1, y1+2, tx1+lmar, y2-1);
	gfxDrawText(tx1+2, ty1+1, gStdColor[0], strupr(buffer), pFont);

	gfxSetColor(gStdColor[22]);
	gfxVLine(tx1+lmar+1, y1, y2);
	lmar+=2;


	// right part
	///////////////////////
	sprintf(buffer, "EDIT %3s", onOff(gArtEd.mode));
	len = gfxGetTextLen(buffer, pFont);

	tx1 = (x1 + (width - rmar - len)) - 1, rmar += (x2 - tx1);

	gfxSetColor(gStdColor[gArtEd.mode ? 12 : 4]);
	gfxFillBox(tx1-1, y1+2, tx1 + len + 1, y2-1); rmar+=2;
	gfxDrawText(tx1, ty1+1, gStdColor[0], buffer, pFont);

	gfxSetColor(gStdColor[22]);
	gfxVLine(tx1-3, y1, y2);
	rmar+=1;
	
	int i = hgltTileCount();
	sprintf(buffer, "Sel %d", i); len = gfxGetTextLen(strupr(buffer), pFont);
	tx1 = (x1 + (width - len - rmar)) - 1; rmar += ((x2 - tx1) - rmar);
	gfxSetColor(gStdColor[5]);
	gfxFillBox(tx1-1, y1+2, tx1 + len + 1, y2-1); rmar+=2;
	gfxDrawText(tx1, ty1+1, (i) ? fade() : gStdColor[0], buffer, pFont);

	gfxSetColor(gStdColor[22]);
	gfxVLine(tx1-3, y1, y2);
	rmar+=2;

	sprintf(buffer, "Pal %d", gTileView.palookup);
	len = gfxGetTextLen(strupr(buffer), pFont);
	tx1 = (x1 + (width - len - rmar)) - 1; rmar += ((x2 - tx1) - rmar);
	gfxSetColor(gStdColor[14]);
	gfxFillBox(tx1-1, y1+2, tx1 + len + 1, y2-1); rmar+=2;
	gfxDrawText(tx1, ty1+1, gStdColor[0], buffer, pFont);

	gfxSetColor(gStdColor[22]);
	gfxVLine(tx1-3, y1, y2);
	rmar+=2;

	if (tilefilenum[gArtEd.nTile] >= 255) sprintf(buffer, "NO FILE");
	else
	{
		ARTFILE* artFile = &gArtFiles[tilefilenum[gArtEd.nTile]];
		getFilename(artFile->path, buffer, FALSE);
		if (artFile->dirty)
			strcat(buffer, "*");
	}

	len = gfxGetTextLen(buffer, pFont);
	tx1 = (x1 + (width - len - rmar)) - 1; rmar += ((x2 - tx1) - rmar);
	gfxSetColor(gStdColor[13]);
	gfxFillBox(tx1-1, y1+2, tx1 + len + 1, y2-1); rmar+=2;
	gfxDrawText(tx1, ty1+1, gStdColor[0], buffer, pFont);

	gfxSetColor(gStdColor[22]);
	gfxVLine(tx1-3, y1, y2);
	rmar+=2;
	

	// mid part
	///////////////////
	gfxSetColor(gStdColor[24]);
	gfxFillBox(x1+lmar+1, y1 + 2, x2 - rmar, y2 - 1);

	if (totalclock < gScreen.msg[0].time)
		gfxDrawText(x1+lmar+2, ty1+1, gStdColor[(totalclock & 32) ? 15 : 18], strupr(gScreen.msg[0].text), pFont);


}

void tileIdxFillFull() {
	for(tileIndexCount = 0; tileIndexCount < gMaxTiles; tileIndexCount++)
		tileIndex[tileIndexCount] = tileIndexCount;
}

void qloadvoxel(int nVoxel)
{
	// this voxel already loaded
	if (voxoff[nVoxel] && voxoff[nVoxel][0] != 0)
		return;

	char *pVox = NULL;
	RESHANDLE hVox;
	int i;
	
	// search voxels in the RFF
	if ((hVox = gSysRes.Lookup(nVoxel, "KVX")) != NULL)
	{
		pVox = (char *)gSysRes.Lock(hVox);
	}
	else if (gMisc.externalModels >= 1)	// search external voxels on the disk
	{
		i = kMaxTiles;
		while(--i >= 0)
		{
			if (tiletovox[i] == nVoxel && extVoxelPath[i])
			{
				if (fileLoadHelper(extVoxelPath[i], (BYTE**)&pVox) <= 0)
				{
					ThrowError("Error loading external model '%s'", extVoxelPath[i]);
					return;
				}
				
				break;
			}
		}
	}
	else
	{
		return;
	}

    
	for (i = 0; i < MAXVOXMIPS; i++)
    {
        int nSize = *((int*)pVox);
#if B_BIG_ENDIAN == 1
        nSize = B_LITTLE32(nSize);
#endif
        pVox += 4;
        voxoff[nVoxel][i] = (intptr_t)pVox;
        pVox += nSize;
    }
}

void CalcPicsiz( int nTile, int x, int y )
{
	int i;
	BYTE n = 0;

	for (i = 2; i <= x; i <<= 1)
		n += 0x01;

	for (i = 2; i <= y; i <<= 1)
		n += 0x10;

	picsiz[nTile] = n;
}

BOOL tileAllocSysTile(short* dest, int w, int h) {

	short nTile = -1;
	if ((nTile = tileGetBlank()) >= 0)
	{
		tileAllocTile(nTile, w, h);
		gSysTiles.add(nTile);
	}

	*dest = nTile;
	return (nTile >= 0);
}



void tileInitSystemTiles() {

	register int i, len;
	register short nTile;
	RESHANDLE hIco;
	
	if (tileAllocSysTile(&nTile, 128, 128))
	{
		gSysTiles.tileViewBg = nTile;
		BYTE* pTile = (BYTE*)waloff[gSysTiles.tileViewBg];
		memset(pTile, gTileView.background, 128*128);
	}

	if (tileAllocSysTile(&nTile, xdim, ydim))
	{
		gSysTiles.drawBuf = nTile;
	}
	
	gSysTiles.gameMirrorStart = 4080;
	gSysTiles.gameMirrorEnd = gSysTiles.gameMirrorEnd = gSysTiles.gameMirrorStart + 16;
	for (i = gSysTiles.gameMirrorStart; i < gSysTiles.gameMirrorEnd; i++)
		gSysTiles.add(i);
	
	gSysTiles.add(2342); // dynamic fire
	
	gSysTiles.angArrow = 4126; BYTE* pArrow = tileLoadTile(2332);
	if (pArrow && tileAllocSysTile(&nTile, tilesizx[2332], tilesizy[2332]));
	{
		// new angle arrow
		len = tilesizx[nTile]*tilesizy[nTile];
		BYTE* pArrowNew = (BYTE*)waloff[nTile];
		gSysTiles.angArrow = nTile;
		memcpy(pArrowNew, pArrow, len);
		artedRotateTile(nTile);
		gSysTiles.add(nTile);
	}
	
	if ((hIco = gGuiRes.Lookup((unsigned int)0, "TGA")) && (nTile = tileGetBlank()) > 0)
	{
		tga2tile((unsigned char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		gSysTiles.icoXmp = nTile;
		gSysTiles.add(nTile);
	}

	if ((hIco = gGuiRes.Lookup((unsigned int)1, "TGA")) && (nTile = tileGetBlank()) > 0)
	{
		tga2tile((unsigned char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		gSysTiles.icoNoTile = nTile;
		gSysTiles.add(nTile);
	}
	
	if ((hIco = gGuiRes.Lookup((unsigned int)2, "TGA")) && (nTile = tileGetBlank()) > 0)
	{
		tga2tile((unsigned char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		panm[nTile].xcenter = tilesizx[nTile] >> 1;
		gSysTiles.wallHglt = nTile;
		gSysTiles.add(nTile);
	}
	
	if ((hIco = gGuiRes.Lookup((unsigned int)3, "TGA")) && (nTile = tileGetBlank()) > 0)
	{
		tga2tile((unsigned char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		gSysTiles.icoVer1 = nTile;
		gSysTiles.add(nTile);
	}
	
	if ((hIco = gGuiRes.Lookup((unsigned int)4, "TGA")) && (nTile = tileGetBlank()) > 0)
	{
		tga2tile((unsigned char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		gSysTiles.icoVer2 = nTile;
		gSysTiles.add(nTile);
	}
}

void tileUninitSystemTiles() {

	int i;
	gSysTiles.remAll();
	if (gSysTiles.tileViewBg)
	{
		tilePurgeTile(gSysTiles.tileViewBg, TRUE);
		tileFreeTile(gSysTiles.tileViewBg);
	}

	if (gSysTiles.angArrow)
	{
		tilePurgeTile(gSysTiles.angArrow, TRUE);
		tileFreeTile(gSysTiles.angArrow);
	}
	
	if (gSysTiles.drawBuf)
	{
		tilePurgeTile(gSysTiles.drawBuf, TRUE);
		tileFreeTile(gSysTiles.drawBuf);
	}
	
	if (gSysTiles.icoXmp)
	{
		tilePurgeTile(gSysTiles.icoXmp, TRUE);
		tileFreeTile(gSysTiles.icoXmp);
	}
	
	if (gSysTiles.icoNoTile)
	{
		tilePurgeTile(gSysTiles.icoNoTile, TRUE);
		tileFreeTile(gSysTiles.icoNoTile);
	}
	
	if (gSysTiles.wallHglt)
	{
		tilePurgeTile(gSysTiles.wallHglt, TRUE);
		tileFreeTile(gSysTiles.wallHglt);
	}
	
	if (gSysTiles.icoVer1)
	{
		tilePurgeTile(gSysTiles.icoVer1, TRUE);
		tileFreeTile(gSysTiles.icoVer1);
	}
	
	if (gSysTiles.icoVer2)
	{
		tilePurgeTile(gSysTiles.icoVer2, TRUE);
		tileFreeTile(gSysTiles.icoVer2);
	}
}

int tileInitFromIni() {
	
	int32_t offset;
	int i, j, nTiles, nFile, hFile, nTile = 0;
	char* tmp = NULL, *fname = NULL;

	memset(tilesizx,		0,   sizeof(tilesizx));
	memset(tilesizy,		0,   sizeof(tilesizy));
 	memset(picanm, 			0,   sizeof(picanm));
	memset(gotpic, 			0,   sizeof(gotpic));
	memset(voxoff,			0,   sizeof(voxoff));
	memset(tilefilenum, 	255, sizeof(tilefilenum));
	memset(tileShade, 		0, 	 sizeof(tileShade));
	memset(voxelIndex, 		-1,  sizeof(voxelIndex));
	memset(surfType,		0,	 sizeof(surfType));
	memset(viewType,		0,	 sizeof(viewType));
	memset(gArtFiles, 		0,   sizeof(gArtFiles));
	
	panm = (PICANM*)picanm; // use PICANM struct

	nFile = 0;
	while ( 1 )
	{
		ARTFILE* artFile =& gArtFiles[nFile];
		sprintf(buffer, "%d", nFile); fname = artFile->path;
		if ((tmp = MapEditINI->GetKeyString("RESOURCES.ART", buffer, "")) != NULL && fileExists(tmp)) sprintf(fname, tmp);
		else sprintf(fname, "%s%03d.ART", gMisc.tilesBaseName, nFile); // default tiles
		if ((hFile = open(fname, O_BINARY | O_RDONLY)) < 0)
			break;
		
		buildprintf("Reading \"%s\"...\n", fname);
		lseek(hFile, sizeof(int32_t) << 1, SEEK_CUR); //artversion, numtiles
		read(hFile, &j, sizeof(int32_t)); artFile->start = j;
		read(hFile, &j, sizeof(int32_t)); artFile->end   = j;

		nTiles = artFile->end - artFile->start + 1;
		if (nTiles < gMaxTiles && nTile < gMaxTiles)
		{
			read(hFile, &tilesizx[artFile->start], nTiles * sizeof(tilesizx[0]));
			read(hFile, &tilesizy[artFile->start], nTiles * sizeof(tilesizy[0]));
			read(hFile, &picanm[artFile->start], nTiles * sizeof(picanm[0]));

			// get current position
			offset = lseek(hFile, 0, SEEK_CUR);
			for (i = artFile->start; i <= artFile->end; i++)
			{
				if (nTile >= gMaxTiles)
					break;

				tilefilenum[i] = (char)nFile;
				tilefileoffs[i] = offset;
				offset += tilesizx[i] * tilesizy[i];
				nTile++;
			}
		}

		close(hFile);
		if (++nFile >= kMaxArtFiles)
			break;
	}
	
	if (nFile <= 0)
		ThrowError("No ART files found!");
	
	nArtFiles = nFile;

	// load extra info from DAT files
	FileLoad(gPaths.surfDAT, surfType, sizeof(surfType));
	FileLoad(gPaths.shadeDAT, tileShade, sizeof(tileShade));
	FileLoad(gPaths.voxelDAT, voxelIndex, sizeof(voxelIndex));

	for (i = 0; i < kMaxTiles; i++)
	{
		CalcPicsiz(i, tilesizx[i], tilesizy[i]); // setup the tile size log 2 array for internal engine use
		if (panm[i].view == kSprViewVox || panm[i].view == kSprViewVoxSpin)
		{
			if (voxelIndex[i] < 0 || !gSysRes.Lookup(voxelIndex[i], "KVX"))
				panm[i].view = kSprViewSingle;
		}
	
		viewType[i] = (BYTE)panm[i].view;
		tileNode[i].ptr = NULL;
		waloff[i] = NULL;
	}
	
	return nArtFiles;

}

void tilePurgeTile(int nTile, BOOL force)
{
	waloff[nTile] = NULL;
	CACHENODE *node = &tileNode[nTile];
	if (node->ptr != NULL)
	{
		Resource::Flush((RESHANDLE)node);
		dassert(node->ptr == NULL);
	}
		
	// mark it as normal tile (non system reserved)
	if (force)
		gSysTiles.rem(nTile);
}

void tilePurgeAll(BOOL force)
{
	for (int i = 0; i < kMaxTiles; i++)
	{
		if (!isSysTile(i) || force)
			tilePurgeTile(i, force);
	}
}

void qloadtile(short nTile) {
	
	int hFile;
	static int nLastTile = 0;

	if (tileNode[nLastTile].lockCount == 0) waloff[nLastTile] = NULL;
	else if (!(nTile >= 0 && nTile < kMaxTiles))
		ThrowError("nTile >= 0 && nTile < kMaxTiles (%d)", nTile);
	
	nLastTile = nTile;

	CACHENODE *node = &tileNode[nTile];

	// already in memory?
	if (node->ptr != NULL)
	{
		waloff[nTile] = (intptr_t)node->ptr;
		if (node->lockCount == 0)
		{
			Resource::RemoveMRU(node);
			Resource::AddMRU(node);
			nLastTile = nTile;
		}

		return;
	}

	int nSize = tilesizx[nTile] * tilesizy[nTile];
	if (nSize <= 0)
		return;

	dassert(node->lockCount == 0);
	node->ptr = Resource::Alloc(nSize);
	waloff[nTile] = (intptr_t)node->ptr;
	Resource::AddMRU(node);
	
	// load the tile from the art file
	if (tilefilenum[nTile] == 255) return;
	else if ((hFile = open(gArtFiles[tilefilenum[nTile]].path, O_BINARY | O_RDONLY)) < 0)
		return;

	lseek(hFile, tilefileoffs[nTile], SEEK_SET);
	read(hFile, node->ptr, nSize);
	close(hFile);
}

BYTE* tileLoadTile( int nTile )
{
	qloadtile(nTile);
	return (waloff[nTile]) ? (BYTE*)waloff[nTile] : NULL;
}

BYTE* tileAllocTile( int nTile, int sizeX, int sizeY, int offX, int offY )
{
	int nSize = sizeX * sizeY; void *p;
	if (nSize <= 0 || nTile >= kMaxTiles)
		return NULL;

	p = Resource::Alloc( nSize );
	dassert(p != NULL);
	tileNode[nTile].lockCount++;
	tileNode[nTile].ptr = p;

	waloff[nTile]   = (intptr_t)p;
	tilesizx[nTile] = (short)sizeX;
	tilesizy[nTile] = (short)sizeY;
 	
	memset(&panm[nTile], 0, sizeof(panm[nTile]));
	panm[nTile].xcenter = ClipRange(offX, -127, 127);
	panm[nTile].ycenter = ClipRange(offY, -127, 127);
	CalcPicsiz(nTile, sizeX, sizeY);
	
	return (BYTE*)waloff[nTile];
}


void tileFreeTile( int nTile )
{
	dassert(nTile >= 0 && nTile < kMaxTiles);

	tilePurgeTile(nTile);
	waloff[nTile] = NULL;
	memset(&panm[nTile], 0, sizeof(panm[nTile]));

	tilesizx[nTile]		= 0;
	tilesizy[nTile]		= 0;
	picsiz[nTile]		= 0;
	
}

void tilePreloadTile( int nTile, int objType )
{
	int nCount = 0;
	switch (viewType[nTile])
	{
		case kSprViewSingle:
			nCount = 1;
			break;

		case kSprViewFull5:
			nCount = 5;
			break;

		case kSprViewFull8:
			nCount = 8;
			break;

		case kSprViewBounce:
			nCount = 2;
			break;

		case kSprViewVox:
		case kSprViewVoxSpin:
			switch (objType) {
				case -1:
				case OBJ_SPRITE:
				case OBJ_FLATSPRITE:
					if (voxelIndex[nTile] >= 0 && voxelIndex[nTile] < kMaxVoxels)
					{
						qloadvoxel(voxelIndex[nTile]);
						if (voxoff[voxelIndex[nTile]][0] != 0)
							break;
					}
					voxelIndex[nTile] = -1;
					panm[nTile].view = kSprViewSingle;
					break;
			}
			break;
	}
	
	while (nCount)
	{
		if (panm[nTile].type != 0)
		{
			for (int i = panm[nTile].frames; i >= 0; i--)
			{
				if (panm[nTile].type == 3) tileLoadTile(nTile - i);
				else tileLoadTile(nTile + i);
			}
		}
		else
			tileLoadTile(nTile);
		
		nTile += 1 + panm[nTile].frames;
		nCount--;
	}
}

int CompareTileFreqs( const void *ref1, const void *ref2 )
{
	return tileHist[*(short *)ref2] - tileHist[*(short *)ref1];
}

int tileBuildHistogram( int type )
{
	int i;
	memset(tileHist, 0, sizeof(tileHist[0]) * kMaxTiles);

	switch ( type )
	{
		case OBJ_WALL:
			for (i = 0; i < numwalls; i++) tileHist[wall[i].picnum]++;
			break;
		case OBJ_MASKED:
			for (i = 0; i < numwalls; i++) tileHist[wall[i].overpicnum]++;
			break;
		case OBJ_CEILING:
			for (i = 0; i < numsectors; i++) tileHist[sector[i].ceilingpicnum]++;
			break;
		case OBJ_FLOOR:
			for (i = 0; i < numsectors; i++) tileHist[sector[i].floorpicnum]++;
			break;
		case OBJ_SPRITE:
		case OBJ_FLATSPRITE:
			for (i = 0; i < kMaxSprites; i++)
			{
				switch (sprite[i].statnum) {
					case kMaxStatus:
					case kStatMarker:
						continue;
				}
				
				if (sprite[i].type != 0 && (sprite[i].cstat & kSprInvisible))
					continue;
				
				BOOL face = ((sprite[i].cstat & kSprRelMask) == kSprFace);
				if ((type == OBJ_SPRITE && !face) || (type == OBJ_FLATSPRITE && face))
					continue;

				tileHist[sprite[i].picnum]++;

			}
			break;
	}

	tileIdxFillFull();
	qsort(tileIndex, gMaxTiles, sizeof(tileIndex[0]), &CompareTileFreqs);

	// find out how many used tiles are in the histogram
	tileIndexCount = 0;
	while (tileHist[tileIndex[tileIndexCount]] > 0 && tileIndexCount < gMaxTiles)
		tileIndexCount++;

	return tileIndex[0];
}

#define kMar 4
int tilePick( int nTile, int nDefault, int type, char* titleArg, char flags) {

	BYTE key, ctrl, alt, shift;
	char* title = gTileView.title;
	char updLev = 2, tileFlags = 0x0, color = 0;
	gTileView.scrSave = NULL; QFONT* pFont = qFonts[1];
	char blank[] = "BLANK"; int blankLen = gfxGetTextLen(blank, pFont) >> 1;
	char fav[]   = "FAV";   int favLen   = strlen(fav)<<2;
	char sys[]   = "LOCK";	int sysLen   = gfxGetTextLen(sys, pFont) >> 1;

	short nDTile = 0, nATile = 0, nVTile = 0;
	int i, j, c, r, x, y, tx, ty, height, width, xrm = 0, yrm = 0, cxrm, cyrm;
	int  nCol, nRow, nCols, nRows, nStart, nCursor = 0, bs, bsx, bsy, ts;
	int x1 = gTileView.wx1, y1 = gTileView.wy1, x2 = gTileView.wx2, y2 = gTileView.wy2;
	int swidth = x2 - x1, sheigh = y2 - y1;
	BOOL drawCol = TRUE;

	hgltTileReset();
	sprintf(title, (titleArg) ? titleArg : "Tile viewer");
	
	MOUSE mouse;
	mouse.ChangeCursor(kBitmapMouseCursor);
	mouse.RangeSet(x1, y1, x2 - pBitmaps[kBitmapMouseCursor]->width, y2);
	
	if (type != OBJ_CUSTOM)
	{
		memset(tilePluIndex, kPlu0, sizeof(tilePluIndex));
		gTileView.palookup = kPlu0;
		tileBuildHistogram(type);
	}
	
	if (gArtEd.mode != kArtEdModeNone)
	{
		switch (type) {
			case OBJ_ALL:
				break;
			case OBJ_CUSTOM:
				if (gArtEd.standalone) break;
				// no break
			default:
				artedUninit();
				gArtEd.mode = kArtEdModeNone;
				break;
		}
		
		if (gArtEd.mode != kArtEdModeNone)
		{
			i = strlen(title);
			sprintf(&title[i], " - %s", gArtEdModeNames[gArtEd.mode]);
		}
	}
	
	if (tileIndexCount == 0) tileIdxFillFull();
	for (i = 0; i < tileIndexCount; i++)
	{
		if (tileIndex[i] != nTile) continue;
		nCursor = i;
		break;
	}

	while ( 1 )
	{
		nTile = nVTile = tileIndex[nCursor];
		if (gArtEd.mode > kArtEdModeBatch)
			nVTile = (short)toolGetViewTile(nTile, gTool.nOctant, NULL, NULL);
		else if (!gArtEd.mode && type == OBJ_CUSTOM)
		{
			if ((i = adjIdxByTileInfo(nTile, adjCountSkips(nTile))) >= 0)
				scrSetMessage("Type %d: %s.", autoData[i].type, gSpriteNames[autoData[i].type]);
		}
		
		gArtEd.nTile = nTile;
		gArtEd.nVTile = nVTile;
		if (updLev > 0)
		{
			if (updLev == 2)
			{
				nCols = gTileView.tilesPerRow;
				//if (swidth < sheigh)
				//{
					bs = swidth / nCols;
					nRows = ClipLow(sheigh / bs, 1);
				//}
				//else
				//{
					//bs = sheigh / nCols;
					//nRows = ClipLow(sheigh / bs, 1);
				//}
				nStart = ClipLow((nCursor - nRows * nCols + nCols) / nCols * nCols, 0);
				updLev--;
			}

			if (!gTileView.scrSave)
			{
				if (gTileView.transBackground && gSysTiles.tileViewBg && gTileView.bglayers)
				{
					gTileView.scrSave = (BYTE*)Resource::Alloc(xdim*ydim);
					for (i = 0; i < gTileView.bglayers; i++)
						rotatesprite(xdim<<15, ydim<<15, 0x10000 << 6, 0, gSysTiles.tileViewBg, 0, 0, (char)(0x20 | 0x04 | 0x01), windowx1, windowy1, windowx2, windowy2);
					
					memcpy(gTileView.scrSave, (void*)frameplace, xdim*ydim);
				}
			}
			else if (!gTileView.transBackground)
			{
				Resource::Free(gTileView.scrSave);
				gTileView.scrSave = NULL;
			}
			
			bsx = bsy = bs;
			if ((i = bs*nCols) < swidth)
			{
				bsx += ((swidth - i) / nCols);
				if ((i = bsx*nCols) < swidth)
					xrm = swidth - i;
			}

			if ((i = bs*nRows) < sheigh)
			{
				bsy += ((sheigh - i) / nRows);
				if ((i = bsy*nRows) < sheigh)
					yrm = sheigh - i;
			}

			ts = ((!gTileView.stretch) ? bs - pFont->height : bs - 4) - 4;
			if (gTileView.showTransColor) tileFlags &= ~0x02;
			else tileFlags |= 0x02;
			updLev = 0;

		}
		
		if (!gTileView.scrSave)
		{
			gfxSetColor(gTileView.background);
			gfxFillBox(x1, y1, x2, y2);
		}
		else
		{
			memcpy((void*)frameplace, gTileView.scrSave, xdim*ydim);
		}

		handleevents();
		keyGetHelper(&key, &ctrl, &shift, &alt);
		if (gArtEd.mode <= kArtEdModeBatch) // draw tile screen
		{
			////////
			if (!key)
				mouse.Read();
			/////////

			toolDrawWindow(x1-1, y1-14, x2+1, y2+15, title, gStdColor[15]);
			tileScrDrawStatus(x1, y2 - 1, x2, y2 + 14);
			i = nStart, x = x1, y = y1, c = nCols, r = nRows, cxrm = xrm, cyrm = yrm;
			while ( 1 )
			{
				drawCol = 1, nDTile = nATile = tileIndex[i];
				if (i < tileIndexCount)
				{
					j = 1;
					if (i == nCursor)
					{
						drawCol = 0;
						DrawRect(x+1, y+1, x+bsx-1, y+bsy-1, fade());
						j++;
					}

					if (tileInHglt(nDTile))
					{
						drawCol = 0; 
						DrawRect(x+j, y+j, x+bsx-j, y+bsy-j, gStdColor[18]);
					}

					if (gTileView.showAnimation)
					{
						if (panm[nDTile].type > 0)
							nATile = (short)ClipRange(nATile+qanimateoffs((short)nDTile,(ushort)0xc000), 0, kMaxTiles - 1);
						if (nATile == 2342)
							FireProcess();
					}

					if ((width = tilesizx[nATile]) <= 0 || (height = tilesizy[nATile]) <= 0)
					{
						if (!isSysTile(nDTile))
							gfxDrawText(x + (bsx >> 1) - blankLen, y + (bsy >> 1) - (pFont->height >> 1), gStdColor[15], gStdColor[4], blank, pFont);
					}
					else if (width < bs && height < bs)
					{
						if (gTileView.stretch)
							tileDrawGetSize(nATile, ts, &width, &height);

						tx = x + ((bsx - width) >> 1); ty = y + ((bsy - height) >> 1);
						tileDrawTile(tx, ty, nATile, !gTileView.stretch ? 0 : ts, tilePluIndex[i], tileFlags, tileShade[nDTile]);
					}
					else
					{
						tileDrawGetSize(nATile, ts, &width, &height);
						tx = x + ((bsx - width) >> 1); ty = y + ((bsy - height) >> 1);
						tileDrawTile(tx, ty, nATile, ts, tilePluIndex[i], tileFlags, tileShade[nDTile]);
					}

					// overlay info
					if (!keystatus[KEY_CAPSLOCK])
					{
						sprintf(buffer, "%d", nDTile); j = gfxGetTextLen(buffer, pFont);
						gfxPrinTextShadow(x + kMar, y + kMar, gStdColor[14], buffer, pFont);
						tx = x + j + 5;
						
						if (gArtEd.mode || gTileView.showInfo)
						{
							if (gDirtTiles[nDTile])
								gfxPrinTextShadow(tx, y + kMar, gStdColor[12], "*", qFonts[3]);
						
							tx = x + bsx - kMar, ty = y + kMar;
							if (panm[nDTile].type && panm[nDTile].frames > 0)
							{
								j = sprintf(buffer, "%s%d", (panm[nDTile].type == 3) ? "<" : (panm[nDTile].type == 1) ? "><" : ">", panm[nDTile].frames) << 2;
								if (gTileView.showAnimation)
								{
									j += sprintf(buffer2, "|%02d", abs(nATile - nDTile)) << 2;
									strcat(buffer, buffer2);
								}
								printextShadowS(tx - j, ty, gStdColor[11], buffer);
								ty = y + 10;
							}
							
							if (panm[nDTile].view)
							{
								j = sprintf(buffer, viewNamesShort[panm[nDTile].view]) << 2;
								printextShadowS(tx - j, ty, gStdColor[3], strupr(buffer));
							}

							if (gArtEd.mode && isSysTile(nDTile))
								gfxDrawText(x + (bsx >> 1) - sysLen, y + (bsy >> 1) - (pFont->height >> 1), gStdColor[0], gStdColor[15], sys, pFont);

							QFONT* pFont = qFonts[3];
							sprintf(buffer, surfNames[surfType[nDTile]]); ty = (y + bsy) - pFont->height - kMar;
							gfxPrinTextShadow(x + kMar, ty, gStdColor[(surfType[nDTile]) ? 21 : 18], strupr(buffer), pFont);
							ty-=pFont->height + kMar;
							
							if (tileShade[nDTile])
							{
								sprintf(buffer, "%+d", tileShade[nDTile]);
								gfxPrinTextShadow(x + kMar, ty, gStdColor[13], strupr(buffer), pFont);
							}
						}

						if (tileInFaves(nDTile) >= 0)
							gfxDrawText(x + bsx - favLen - kMar, y + bsy - qFonts[3]->height - kMar, gStdColor[12], fav, qFonts[3]);

					}
				}
				
				if (drawCol)
					DrawRect(x+1, y+1, x+bsx-1, y+bsy-1, gTileView.background ^ 2);

				if (--c)
				{
					x+=bsx;
					if (--cxrm > 0)
						x++;
				}
				else if (--r)
				{
					y+=bsy, x = x1, c = nCols, cxrm = xrm;
					if (--cyrm > 0)
						y++;
				}
				else break;
				i++;
			}

			

			// process mouse
			/////////////////////////////////////////////////////////////////////////////////////////
			mouse.Draw();
			if (!key)
			{
				if (mouse.release & 1) key = KEY_ENTER;
				else if (mouse.release & 4) key = (gArtEd.mode == kArtEdModeBatch) ? KEY_TILDE : KEY_F1;
				else if (gTimers.tileScroll <= totalclock)
				{
					if (mouse.hold & 2)
					{
						if (nRow <= 0) key = KEY_UP;
						else if (nRow >= nRows - 1)
							key = KEY_DOWN;
					}
					else if (mouse.wheel)
					{
						if (mouse.wheel > 0) nStart += nCols;
						else if (mouse.wheel < 0)
							nStart -= nCols;
						
						if (mouse.hold & 4)
							mouse.hold &= ~4;
					}
					
					gTimers.tileScroll = totalclock + 5;
				}
				
				if (mouse.dX2 || mouse.dY2 || mouse.wheel)
				{
					nStart = ClipRange(nStart, 0, tileIndexCount - 1);
					nCol = ClipRange(mouse.X/bsx, 0, nCols-1); nRow = ClipRange(mouse.Y/bsy, 0, nRows-1);
					nCursor = ClipRange((nRow*nCols)+nStart+nCol, nStart, ClipHigh(((nCols*nRows)+nStart)-1, tileIndexCount-1));
				}
			}
		}
		else if (gArtEd.mode >= kArtEdModeTile2D) // draw selected tile
		{
			j = sheigh / 3;

			PICANM* pInfo = &panm[nTile];
			QFONT* pFont = gTool.pFont;
			color = gStdColor[20];
			
			gfxSetColor(gStdColor[28]);
			gfxFillBox(x1, y1, x2, y2);
			toolDrawPixels(gTool.hudPixels, gStdColor[29]);
			toolDrawCenter(&gArtEd.origin, color, j, j, gTool.centerPixels); // draw dots before tile
			
			if (tilesizx[nVTile] && tilesizy[nVTile])
				artedDrawTile(nTile, gTool.nOctant, tileShade[nTile]);

			toolDrawCenter(&gArtEd.origin, color, 6, 6, 1);	// draw crosshair after tile
 			toolDrawCenterHUD(&gArtEd.origin, nTile, nVTile, j, gTool.nOctant, color);
			toolDrawWindow(0, 0, xdim, ydim, title, gStdColor[15]);
			tileScrDrawStatus(x1, y2 - 1, x2, y2 + 14);

			// bottom part first
			color = gStdColor[16];
			y = y1 + sheigh - pFont->height - 2, x = x1 + 4;
			sprintf(buffer, "Type: %s  Frames: %02d  Delay: %02d", animNames[pInfo->type], pInfo->frames, pInfo->speed);
			gfxPrinTextShadow(x, y, color, strupr(buffer), pFont);

			y -= 4;
			gfxSetColor(gStdColor[6]);
			gfxHLine(y, x, swidth - 4);
			y -= (pFont->height + 2);

			sprintf(buffer, "Animation");
			gfxPrinTextShadow(x, y, color, strupr(buffer), pFont);
			/////////////////////////////

			// top part then
			y = y1 + 4;

 			sprintf(buffer, "General: Surface = %s  View = %s  Shade = %+04d",
				surfNames[surfType[nTile]], viewNames[viewType[nTile]], tileShade[nTile]);
			gfxPrinTextShadow(x, y, color, strupr(buffer), pFont);
			
			sprintf(buffer, "RFF Voxel ID: %d", voxelIndex[nTile]);
			if (voxelIndex[nTile] >= 0)
			{
				char kvxExt[] = "KVX";
				RESHANDLE hVox = gSysRes.Lookup(voxelIndex[nTile], kvxExt);
				if (hVox)
				{
					memset(buffer2, 0, sizeof(buffer2));
					sprintf(buffer2, "(%s.%s)", hVox->name, kvxExt);
					strcat(buffer, " ");
					strcat(buffer, buffer2);
				}
			}
			i = gfxGetTextLen(strupr(buffer), pFont);
			gfxPrinTextShadow(x2 - i - 4, y, color, buffer, pFont);

			y += (pFont->height + 2);
			gfxSetColor(gStdColor[6]);
			gfxHLine(y, x, swidth - 4);
			y+=4;

			sprintf(buffer, "View tile: %d", nVTile);
			gfxPrinTextShadow(x, y, color, strupr(buffer), pFont);
			/////////////////////////////

		}
		OSD_Draw();
		showframe();
		updateClocks();
		
		if (artedProcessEditKeys(key, ctrl, alt, shift))
		{
			keyClear();
			key = 0;
		}	

		switch (key) {
			case KEY_F1:
				tileShowInfoWindow(nVTile);
				break;
			case KEY_V:
				if (gArtEd.mode >= kArtEdModeTile2D)
				{
					gArtEd.mode = kArtEdModeBatch;
					sprintf(title, gArtEdModeNames[gArtEd.mode]);
				}
				else if (type == OBJ_CUSTOM || tileIndexCount >= kMaxTiles) break;
				nCursor = tileIndex[nCursor];
				tileIdxFillFull();
				break;
			case KEY_SPACE:
				i = gArtEd.mode;
				switch (gArtEd.mode) {
					case kArtEdModeNone:
						artedInit();
						gArtEd.mode = kArtEdModeBatch;
						tileIdxFillFull();
						nCursor = nTile;
						break;
					default:
						hgltTileReset();
						gArtEd.mode = IncRotate(gArtEd.mode, kArtEdModeMax);
						gArtEd.mode = ClipRange(gArtEd.mode, gArtEd.modeMin, gArtEd.modeMax);
						artedViewZoomReset();
						gTool.nOctant = 0;
						break;
				}
				if (i != gArtEd.mode) sprintf(title, gArtEdModeNames[gArtEd.mode]);
				break;
			case KEY_ESC:
				switch (gArtEd.mode) {
					case kArtEdModeTile2D:
						gArtEd.mode = kArtEdModeBatch;
						sprintf(title, gArtEdModeNames[gArtEd.mode]);
						break;
					case kArtEdModeBatch:
						if (hgltTileCount() > 0)
						{
							hgltTileReset();
							scrSetMessage("Highlight reset.");
							break;
						}
						// no break
					case kArtEdModeNone:
						if (gTileView.scrSave) Resource::Free(gTileView.scrSave);
						tileIndexCursor = -1;
						nTile = nDefault;
						title[0] = '\0';
						return nTile;
				}
				break;
			case KEY_ENTER:
			case KEY_PADENTER:
				if (gArtEd.standalone) break;
				tileIndexCursor = (short)nCursor;
				nTile = tileIndex[nCursor];
				if (gTileView.scrSave) Resource::Free(gTileView.scrSave);
				if (tilesizx[nTile] && tilesizy[nTile]) return nTile;
				else if (flags & kTilePickFlgAllowEmpty) return nTile;
				else return nDefault;
			case KEY_PAGEUP:
			case KEY_PAGEDN:
				if (gArtEd.mode > kArtEdModeBatch)
				{
					i = (key == KEY_PAGEUP) ? -1 : 1;
					nCursor = ClipRange(nCursor + i, 0, tileIndexCount - 1);
					scrSetMessage("Tile #%d.", tileIndex[nCursor]);
				}
				// no break
			case KEY_HOME:
			case KEY_F12:
				if (gArtEd.mode > kArtEdModeBatch) break;
				switch (key) {
					case KEY_PAGEUP:
						if (nCursor - nRows * nCols < 0) break;
						nStart = ClipLow(nStart - (nRows * nCols), 0);
						nCursor -= nRows * nCols;
						break;
					case KEY_PAGEDN:
						if (nCursor + nRows * nCols >= tileIndexCount) break;
						nCursor += nRows * nCols;
						nStart += nRows * nCols;
						break;
					case KEY_HOME:
						nCursor = 0;
						break;
					case KEY_F12:
						tileScrOptionsDlg();
						updLev = 1;
						break;
				}
				break;
			case KEY_PADSTAR:
			case KEY_PADSLASH:
				j = gTileView.tilesPerRow, i = (key == KEY_PADSTAR) ? -1 : 1;
				nCols = gTileView.tilesPerRow = (BYTE)ClipRange(gTileView.tilesPerRow + i, 3, 14);
				scrSetMessage("Show %d tiles per row.", gTileView.tilesPerRow);
				updLev = 2;
				break;
			case KEY_UP:
			case KEY_DOWN:
			case KEY_RIGHT:
			case KEY_LEFT:
				if (gArtEd.mode > kArtEdModeBatch) break;
				switch (key) {
					case KEY_UP:
						if (nCursor - nCols >= 0) nCursor -= nCols;
						break;
					case KEY_DOWN:
						if (nCursor + nCols < tileIndexCount) nCursor += nCols;
						break;
					case KEY_LEFT:
						if (nCursor - 1 >= 0) nCursor--;
						break;
					case KEY_RIGHT:
						if (nCursor + 1 < tileIndexCount) nCursor++;
						break;
				}
				break;
			case KEY_G:
				nTile = ClipLow(GetNumberBox("Goto tile", 0, tileIndex[nCursor]), 0);
				if (nTile == tileIndex[nCursor]) break;
				for (i = 0; i < tileIndexCount && tileIndex[i] != nTile; i++);
				switch (type) {
					case OBJ_CUSTOM:
						if (i >= tileIndexCount) break;
						nCursor = i;
						break;
					default:
						if (i >= tileIndexCount && i < gMaxTiles)
						{
							tileIdxFillFull();
							for (i = 0; i < tileIndexCount && tileIndex[i] != nTile; i++);
						}
						nCursor = ClipHigh(i, tileIndex[gMaxTiles - 1]);
						break;
				}
				break;
			case KEY_P: // select PLU for tiles to preview it
				sprintf(buffer, "Preview palookup"); j = gTileView.palookup;
				if (type == OBJ_CUSTOM) break;
				else if (ctrl) j = kPlu0;
				else if (shift & 0x01) j = getClosestId(j, kPluMax - 1, "PLU", TRUE);
				else if (shift & 0x02) j = getClosestId(j, kPluMax - 1, "PLU", FALSE);
				else if (alt) j = GetNumberBox(buffer, j, j);
				memset(tilePluIndex, j, sizeof(tilePluIndex));
				scrSetMessage("%s: %d.", buffer, j);
				gTileView.palookup = (char)j;
				break;
			case KEY_Q:
				if (gArtEd.mode >= kArtEdModeBatch) break;
				scrSetMessage("Show %s tiles.", gSearchStatNames[type = IncRotate(type, OBJ_SECTOR)]);
				tileBuildHistogram(type);
				nCursor = nStart = 0;
				updLev = 2;
				break;
			case KEY_F: // add or remove tile from favorites
				if ((i = tileInFaves(nTile)) >= 0)
				{
					if (Confirm("Remove this tile from the favorites?"))
					{
						favoriteTileRemove(nTile);
						scrSetMessage("Tile #%d has been removed from the favorites.", nTile);
					}
				}
				else if (gFavTilesC < kMaxFavoriteTiles - 1 && (i = favoriteTileAddSimple(temptype, (short)nTile)) >= 0)
				{
					scrSetMessage("Tile #%d added as type #%d.", gFavTiles[i].pic, gFavTiles[i].type);
				}
				break;

		}

		while (nCursor < nStart) nStart -= nCols;
		while (nStart + nRows * nCols <= nCursor) nStart += nCols;
		if (key != 0)
			keyClear();

	}
}


BYTE tileGetSurfType( int nHit )
{
	int nHitType = nHit & 0xE000;
	int nHitIndex = nHit & 0x1FFF;
	switch ( nHitType )
	{
		case 0x4000:
			return surfType[sector[nHitIndex].floorpicnum];

		case 0x6000:
			return surfType[sector[nHitIndex].ceilingpicnum];

		case 0x8000:
			return surfType[wall[nHitIndex].picnum];

		case 0xC000:
			return surfType[sprite[nHitIndex].picnum];
	}
	return 0;
}

short tileSearchFreeRange(int range) {

	register short j = 0;
	register short i = kMaxTiles;
	
	while(--i >= 0)
	{
		if (gSysTiles.isBusy(i) || tilesizx[i]) j = 0;
		else if (j == range) return i;
		else j++;
	}
	
	return -1;

}

short tileGetBlank() { return tileSearchFreeRange(0); }

int tileCountColors(int nTile) {

	BYTE* pTile = NULL;
	if ((pTile = tileLoadTile(nTile)) == NULL)
		return 0;
	
	return countUniqBytes(pTile, tilesizx[nTile]*tilesizy[nTile]);
}

BOOL tileHasColor(int nTile, char color)
{
	
	BYTE* pTile = (BYTE*)waloff[nTile];
	if (!pTile)
		return FALSE;

	int len = tilesizx[nTile]*tilesizy[nTile];
	for (int i = 0; i < len; i++) {
		if (pTile[i] == color)
			return TRUE;

	}

	return FALSE;
}

BYTE tileGetMostUsedColor(int nTile, short noColor)
{
	return mostUsedColor(tileLoadTile(nTile), tilesizx[nTile]*tilesizy[nTile], noColor);
}

int tileGetPic(int nTile)
{
	if (panm[nTile].type > 0)
		nTile = (short)ClipRange(nTile+qanimateoffs(nTile, 0xc000), 0, kMaxTiles - 1);
	
	return nTile;
}



void tileDrawGetSize(short pic, int size, int* width, int* height, uint32_t* du, uint32_t* dv) {
		
	dassert(pic >= 0 && pic < kMaxTiles);
	
	int uSize = tilesizx[pic]; int vSize = tilesizy[pic];
	*width = uSize, *height = vSize;
	*du = *dv = 0;
	
	if (uSize > 0 && vSize > 0 && size > 0)
	{
		int32_t nShift = 30 - max(picsiz[pic] >> 4, picsiz[pic] & 15);
		
		// wider
		if (uSize > vSize)
		{
			*du = divscale16(uSize, size);
			*dv = divscale(uSize, size, nShift);
			*height = size * vSize / uSize;
			*width = size;
		}
		// taller
		else
		{
			*du = divscale16(vSize, size);
			*dv = divscale(vSize, size, nShift);
			*height = size;
			*width = size * uSize / vSize;
		}
	}
	
}

void tileDrawGetSize(short pic, int size, int* width, int* height) {
	
	int wh, hg; uint32_t du, dv;
	tileDrawGetSize(pic, size, &wh, &hg, &du, &dv);
	*width  = wh, *height = hg;
	
}



void tileDrawTile(int x, int y, short pic, int size, short plu, char flags, schar shade) {

#if USE_POLYMOST
	if (getrendermode() >= 3)
		return;
#endif
	
	BOOL trans = (BOOL)(flags & 0x02);
	intptr_t pTile = (intptr_t)tileLoadTile(pic);
	intptr_t pScreen = (intptr_t)frameplace + ylookup[y] + x;
	int i, k, nShift = 30 - max(picsiz[pic] >> 4, picsiz[pic] & 15), height = 0, width = 0;
	int32_t uSize = tilesizx[pic]; int vSize = tilesizy[pic];
	uint32_t u, du, dv;

	if (!pTile || uSize <= 0 || vSize <= 0) return;
	else if (trans) setupmvlineasm(nShift);
	else setupvlineasm(nShift);

	for (i = 0; i < 4; i++)
		palookupoffse[i] = (intptr_t)(palookup[plu] + (qgetpalookup(0, shade) << 8));

	if (!size)
	{
		bufplce[3] = pTile - vSize;
		vince[0] = vince[1] = vince[2] = vince[3] = 1 << nShift;
		for (u = 0; u + 3 < uSize; u += 4)
		{
			bufplce[0] = bufplce[3] + vSize;
			bufplce[1] = bufplce[0] + vSize;
			bufplce[2] = bufplce[1] + vSize;
			bufplce[3] = bufplce[2] + vSize;
			memset(vplce, 0, sizeof(vplce));

			if (trans) mvlineasm4(vSize, (char*)pScreen);
			else vlineasm4(vSize, (char*)pScreen);
			pTile += 4 * vSize;
			pScreen += 4;
		}

		while(u++ < uSize)
		{

			if (trans) mvlineasm1(1 << nShift, (void*)palookupoffse[0], vSize - 1, 0, (void*)pTile, (void*)pScreen);
			else vlineasm1(1 << nShift, (void*)palookupoffse[0], vSize - 1, 0, (void*)pTile, (void*)pScreen);
			pTile += vSize;
			pScreen++;
		}

		return;
	}
	
	tileDrawGetSize(pic, size, &width, &height, &du, &dv);
	if (height <= 0 || width <= 0)
		return;

	vince[0] = vince[1] = vince[2] = vince[3] = dv;
	for (k = 0, u = 0; k + 3 < width; k += 4)
	{
		for (i = 0; i < 4; i++, u += du)
			bufplce[i] = pTile + vSize * (u >> 16);

		memset(vplce, 0, sizeof(vplce));
		if (trans) mvlineasm4(height, (char*)pScreen);
		else vlineasm4(height, (char*)pScreen);
		pScreen += 4;
	}

	while (k++ < width)
	{
		pTile = (intptr_t)waloff[pic] + vSize * (u >> 16);

		if (trans) mvlineasm1(dv, (void*)palookupoffse[0], height - 1, 0, (void*)pTile, (void*)pScreen);
		else vlineasm1(dv, (void*)palookupoffse[0], height - 1, 0, (void*)pTile, (void*)pScreen);
		pScreen++;
		u += du;
	}

}

int tileExists(BYTE* image, int wh, int hg) {
	
	int i, len = wh*hg;
	for (i = 0; i < gMaxTiles; i++)
	{
		if (wh != tilesizx[i] || hg != tilesizy[i]) continue;
		else if (tileLoadTile(i) && memcmp((void*)waloff[i], image, len) == 0)
			return i;
	}
	
	return -1;
}


void tileShowInfoWindow(int nTile) {

	int x = 6, y = 4, len;
	BYTE* pTile = tileLoadTile(nTile);
	sprintf(buffer, "Tile #%d properties", nTile);
	Window dialog(0, 0, 162, ydim, buffer);
	
	y+=4;
	dialog.Insert(new Label(x, y, ">BASICS", kColorRed));
	y+=12;


	sprintf(buffer, "Width: %dpx", tilesizx[nTile]);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;

	sprintf(buffer, "Height: %dpx", tilesizy[nTile]);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;

	sprintf(buffer, "Surface: %s  [%d]", surfNames[surfType[nTile]], surfType[nTile]);
	dialog.Insert(new Label(x, y, buffer));

	if (gMapLoaded) {

		y+=10;

		sprintf(buffer, "Used: %d time(s)", tileHist[nTile]);
		dialog.Insert(new Label(x, y, buffer));

	}

	y+=10;
	
	
	getFilename(gArtFiles[tilefilenum[nTile]].path, buffer2, TRUE);
	sprintf(buffer, "ART File: %s", buffer2);
	dialog.Insert(new Label(x, y, buffer));
	
	
	y+=20;
	dialog.Insert(new Label(x, y, ">TILE VIEW", kColorRed));
	y+=12;

	sprintf(buffer, "Type: %s  [%d]", viewNames[viewType[nTile]], viewType[nTile]);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;

	if (panm[nTile].view >= kSprViewVox)
	{
		sprintf(buffer, "Voxel ID: %d", voxelIndex[nTile]);
		if (voxelIndex[nTile] >= 0) {

			strcat(buffer, " ");
			sprintf(buffer2, "(%s)", (isExternalModel(nTile)) ? "disk" : "rff");
			strcat(buffer, buffer2);

		}

		dialog.Insert(new Label(x, y, buffer));

		y+=10;
	}
	
	
	sprintf(buffer, "X-offset: %dpx", panm[nTile].xcenter);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;

	sprintf(buffer, "Y-offset: %dpx", panm[nTile].ycenter);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;

	sprintf(buffer, "Shade: %d", tileShade[nTile]);
	dialog.Insert(new Label(x, y, buffer));

	y+=20;
	dialog.Insert(new Label(x, y, ">ANIMATION", kColorRed));
	y+=12;

	if (panm[nTile].type)
	{
		sprintf(buffer, "Type: %s  [%d]", animNames[panm[nTile].type], panm[nTile].type);
		dialog.Insert(new Label(x, y, buffer));

		y += 10;

		sprintf(buffer, "Frames: %d", panm[nTile].frames);
		dialog.Insert(new Label(x, y, buffer));

		y+=10;

		sprintf(buffer, "Delay: %d", panm[nTile].speed);
		dialog.Insert(new Label(x, y, buffer));

	}
	else
	{
		dialog.Insert(new Label(x, y, "Not present"));
	}

	y+=20;
	dialog.Insert(new Label(x, y, ">MISCELLANEOUS", kColorRed));
	y+=12;

	int color = tileCountColors(nTile);
	sprintf(buffer, "Total colors used: %d", color);
	dialog.Insert(new Label(x, y, buffer));

	y+=10;
	
	if (color)
	{
		color = tileGetMostUsedColor(nTile);
		sprintf(buffer, "Most used color:");
		dialog.Insert(new Label(x, y, buffer));

		len = gfxGetTextLen(buffer, pFont) + 5; int siz = pFont->height;
		dialog.Insert(new Shape(x + len - 1, y - 1, siz + 2, siz + 2, gStdColor[18]));
		dialog.Insert(new Shape(x + len, y, siz, siz, color));

		sprintf(buffer, "[%d]", color);
		dialog.Insert(new Label(x + len + siz + 4, y, buffer));

		y+=10;
		
		int xnice = (pow2long[picsiz[nTile] & 15] == tilesizx[nTile]);
		int ynice = (pow2long[picsiz[nTile] >> 4] == tilesizy[nTile]);
		sprintf(buffer, "Power of two sized: %s", yesNo(xnice && ynice));
		dialog.Insert(new Label(x, y, buffer));

		y+=10;
	}
	
	if (pTile)
	{
		sprintf(buffer, "CRC32: %x", crc32once(pTile, tilesizx[nTile]*tilesizy[nTile]));
		dialog.Insert(new Label(x, y, buffer));
		y+=10;
	}
	
	sprintf(buffer, "Favorited: %s", yesNo(tileInFaves(nTile) >= 0));
	dialog.Insert(new Label(x, y, buffer));

	y+=20;
	dialog.Insert(new TextButton(x, y, 60, 20,  "&Ok",     mrOk));

	y+=44;

	dialog.height = y;

	ShowModal(&dialog);

}

/* void tileDrawTile2(int x, int y, int ang, short pic, int size, short plu, char flags, schar shade)
{
	rotatesprite(x << 16, y << 16, size, ang, pic, shade,
	(char)plu, 0, windowx1, windowy1, windowx2, windowy2);
} */