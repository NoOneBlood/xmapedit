/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Game GUI based directory explorer that supports thumbnail preview generation.
// Includes simple thumbnail cache system.
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
#include "gui.h"
#include "gfx.h"
#include "screen.h"
#include "xmpstub.h"
#include "xmpexplo.h"
#include "xmpmisc.h"
#include "xmpstr.h"
#include "tile.h"
#include "prefabs.h"
#include "xmparted.h"
#include "editor.h"
#include "hglt.h"
#include "xmpview.h"
#include "img2tile.h"
#include "xmpqav.h"
#include "crc32.h"
#include <direct.h>

#define kThumbnailCacheFile "xmapedit/thumbs.xch"

typedef char (*THUMBDRAWFUNC)(char* filepath, int nTile, int width, int height, int bg);

enum
{
	kThumbSmall			= 0x01,
	kThumbLarge			= 0x02,
	kThumbCache			= 0x04,
	kThumbForceStretch	= 0x08,
};
struct REGISTERED_FILE_TYPE
{
	const char* type;						// file extension
	const char* description;				// brief description
	THUMBDRAWFUNC pThumbFunc;				// ptr to function that gets thumbnail
	unsigned int thumbWidth			: 16;	// thumbnail max width
	unsigned int thumbHeight		: 16;	// thumbnail max height
	unsigned int thumbType			: 8;	// thumbnail flags
	signed int thumbBg				: 10;	// thumbnail background std color
	signed int typeColor			: 10;	// extension text std color
	signed int typeBgColor			: 10;	// extension background std color
};

/** Thumbnail generation functions
********************************************************************************/
static char getThumbnail_PICS(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_PALS(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_QAV(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_SEQ(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_ART(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_MAP(char* filepath, int nTile, int wh, int hg, int bg);
static char getThumbnail_PFB(char* filepath, int nTile, int twh, int thg, int bg);
static void helperAllocThumb(int nTile, int sizeX, int sizeY, int bg);


/** Sorting helpers
********************************************************************************/
static int qsSortDirMoveUp(SCANDIRENT* ref1, SCANDIRENT* ref2);
static int qsSortByType(SCANDIRENT* ref1, SCANDIRENT* ref2);
static int qsSortByName(SCANDIRENT* ref1, SCANDIRENT* ref2);


/** Supported file types
********************************************************************************/
REGISTERED_FILE_TYPE gRegTypes[] =
{
	{".MAP", "BUILD Engine board", 					getThumbnail_MAP, 	256,	256,	0x0F,	28, 			kColorYellow,		kColorGreen			},
	{".PFB", "XMAPEDIT prefab",						getThumbnail_PFB,	200,	180,	0x0F,	kColorMagenta, 	kColorWhite, 		kColorRed			},
	{".PAL", "Palette",								getThumbnail_PALS, 	64,		64, 	0x0F,	28, 			kColorWhite, 		kColorBlue			},
	{".PLU", "BLOOD palookup",						getThumbnail_PALS, 	64,		64, 	0x0F,	28, 			kColorYellow, 		kColorBlue			},
	{".DAT", NULL, 									getThumbnail_PALS, 	64,		64, 	0x0F,	28, 			kColorWhite, 		kColorBlue			},
	{".SEQ", "BLOOD world animation",				getThumbnail_SEQ, 	256,	256,	0x07,	kColorMagenta, 	kColorLightCyan, 	kColorBlack			},
	{".QAV", "BLOOD interface animation",			getThumbnail_QAV, 	320,	200,	0x0F,	28, 			kColorLightRed, 	kColorBlack			},
	{".ART", "BUILD Engine graphics",				getThumbnail_ART, 	256,	256,	0x07,	kColorMagenta, 	kColorWhite, 		kColorBrown			},
	{".PCX", "ZSoft Paintbrush",					getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	{".TGA", "Truevision Targa",					getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	{".QBM", "Q-Studios bitmap",					getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	{".CEL", "CEL image",							getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	#ifdef USE_KPLIB
	{".PNG", "Portable Network Graphics",			getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta,	kColorBrown, 		kColorYellow		},
	{".DDS", "Direct Draw Surface",					getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	{".BMP", "Windows or OS/2 Bitmap",				getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta, 	kColorBrown, 		kColorYellow		},
	{".JPG", "JPEG image",							getThumbnail_PICS, 	256,	256, 	0x07,	kColorMagenta,	kColorBrown, 		kColorYellow		},
	#endif
};


/** A simple thumbnail cache system
********************************************************************************/
enum
{
	CACHE_PRM	= 0x01,
};
class ThumbCache
{
	private:
		struct CACHE_INFO
		{
			uint32_t id;
			uint16_t wh;
			uint16_t hg;
		};
		unsigned int operable		: 1;
		unsigned int permanent		: 1;
		char filename[BMAX_PATH];
		IDLIST *offset, *id;
		int hFile, maxSize;
	public:
		ThumbCache(char* filepath = NULL, int flags = 0x0, int nMaxSize = 128 * 1024 * 1024);
		~ThumbCache(void);
		char Add(uint32_t nID, int nTile);
		char Get(uint32_t nID, int nTile);
		char Offset(uint32_t nID);
		char SetSize(int nSize);
		void Erase(void);
		char Load();
		char Test();
};

ThumbCache::ThumbCache(char* file, int flags, int nMaxSize)
{
	permanent 		= (!isempty(file) && (flags & CACHE_PRM) > 0);
	maxSize			= nMaxSize;
	offset			= new IDLIST(true);
	id				= new IDLIST(true);
	hFile			= -1;
	operable 		= 0;
	
	if (!isempty(file))	strcpy(filename, file);
	else if (!tmpnam(filename))
		return;

	hFile = open(filename, O_CREAT|O_BINARY|O_RDWR, S_IREAD|S_IWRITE);

	if (Test())
	{
		operable = 1;
		SetSize(nMaxSize);
		int nLen = filelength(hFile);
		if ((!permanent && nLen > 0) || (maxSize && nLen > maxSize) || (permanent && nLen > 0 && !Load()))
			Erase();
	}
	else
	{
		operable = 0;
		if (hFile >= 0)
			close(hFile);

		hFile = -1;
	}
}

ThumbCache::~ThumbCache(void)
{
	if (hFile >= 0)
		close(hFile);

	if (!permanent)
		unlink(filename);

	delete(offset);
	delete(id);
}

// simple read/write test just to be sure
////////////////////////////////////////////
char ThumbCache::Test()
{
	const int kTestValue	= 1234;
	const int kSiz			= sizeof(kTestValue);
	int nTest 				= kTestValue;
	int pos;

	if (hFile >= 0 && (pos = lseek(hFile, 0, SEEK_END)) >= 0)
	{
		if (write(hFile, &nTest, kSiz) == kSiz && lseek(hFile, -kSiz, SEEK_CUR) == pos)
		{
			if (read(hFile, &nTest, kSiz) == kSiz && nTest == kTestValue)
			{
				if (chsize(hFile, pos) == 0 && lseek(hFile, pos, SEEK_SET) == pos)
					return 1;
			}
		}
	}

	return 0;
}

// load contents of an existing cache file
////////////////////////////////////////////
char ThumbCache::Load()
{
	int nOfs = lseek(hFile, 0, SEEK_SET);
	int nLen = filelength(hFile);
	CACHE_INFO inf;

	while(nOfs < nLen)
	{
		if (read(hFile, &inf, sizeof(inf)) == sizeof(inf))
		{
			id->Add(inf.id);
			offset->Add(nOfs);

			nOfs+=sizeof(inf)+(inf.wh*inf.hg);
			if (lseek(hFile, nOfs, SEEK_SET) == nOfs)
				continue;
		}

		break;
	}

	return (nOfs == nLen);
}

// set file pointer to cacheID
////////////////////////////////////////////
char ThumbCache::Offset(uint32_t nID)
{
	if (operable)
	{
		int32_t i, numItems, nTop, nBot;
		int32_t* pOffs	= offset->GetPtr();
		int32_t* pID	= id->GetPtr();

		numItems = offset->GetLength();
		nTop = nBot = numItems >> 1;
		i = numItems;
		
		while(i-- >= 0)
		{
			if (nBot < numItems && ((i % 2) || nTop < 0))
			{
				if (pID[nBot] == (int32_t)nID)
					return (lseek(hFile, pOffs[nBot], SEEK_SET) == pOffs[nBot]);
				
				nBot++;
			}
			else if (--nTop >= 0)
			{
				if (pID[nTop] == (int32_t)nID)
					return (lseek(hFile, pOffs[nTop], SEEK_SET) == pOffs[nTop]);
			}
		}
	}

	return 0;
}

// set the new cache size
///////////////////////////////////////////////
char ThumbCache::SetSize(int nSize)
{
	if (operable)
	{
		if (filelength(hFile) > nSize)
			Erase();

		maxSize = nSize;
	}

	return 0;
}

// truncate the cache to zero
///////////////////////////////////////////////
void ThumbCache::Erase(void)
{
	if (operable)
		chsize(hFile, 0);
	
	delete(offset); delete(id);
	offset	= new IDLIST(true);
	id		= new IDLIST(true);
}

// append a new cache content
///////////////////////////////////////////////
char ThumbCache::Add(uint32_t nID, int nTile)
{
	if (operable)
	{
		if (maxSize && filelength(hFile) >= maxSize)
			Erase();
		
		CACHE_INFO inf;
		int nPos, t;

		inf.id	= nID;
		inf.wh	= tilesizx[nTile];
		inf.hg	= tilesizy[nTile];

		t = inf.wh*inf.hg;
		nPos = lseek(hFile, 0, SEEK_END);
		if (write(hFile, &inf, sizeof(inf)) == sizeof(inf) && write(hFile, (void*)waloff[nTile], t) == t)
		{
			id->Add(inf.id);
			offset->Add(nPos);
			return 1;
		}

		operable = 0;
	}

	return 0;
}

// get cache content
///////////////////////////////////////////////
char ThumbCache::Get(uint32_t nID, int nTile)
{
	if (operable)
	{
		int16_t res[2];
		if (Offset(nID))
		{
			lseek(hFile, sizeof(nID), SEEK_CUR);
			read(hFile, &res, sizeof(res));

			helperAllocThumb(nTile, res[0], res[1], -1);
			read(hFile, (void*)waloff[nTile], res[0]*res[1]);
			return 1;
		}
	}

	return 0;
}

/** File picker GUI element
********************************************************************************/
class FilePick : public Container
{
	private:
		unsigned int tmbSizeSTall		: 16;
		unsigned int tmbSizeSWide		: 16;
		unsigned int tmbSizeLTall		: 16;
		unsigned int tmbSizeLWide		: 16;
		unsigned int scrollerHg			: 16;
		unsigned int scollAreaHg		: 16;
		unsigned int scrollSize			: 8;
		unsigned int icoSize			: 16;
		unsigned int namePad			: 16;
		unsigned int gotThumbS			: 3;
		unsigned int gotThumbL			: 3;
		unsigned int scrlTotRows		: 16;
		unsigned int colHg				: 16;
		unsigned int colWh				: 16;
		QBITMAP *pPreviewLayer;
		QBITMAP* pIcoLayer;
		QBITMAP* pMaskLayer;
		int nThumbDrawTile;
	public:
		unsigned int largePreviewSize	: 16;
		unsigned int updateIcons		: 1;
		unsigned int autoRows			: 1;
		unsigned int autoCols			: 1;
		unsigned int showThumbs			: 1;
		unsigned int stretchPreview		: 1;
		unsigned int numfiles;
		int nStart, nCols, nRows, value;
		int nCursor, nCursorOld;
		EditText* pNameInput;
		ThumbCache* pCache;
		SCANDIRENT* files;
		//------------------------------------------------------
		FilePick(int left, int top, int width, int height, FILEBROWSER_PREFS* pPrefs);
		~FilePick();
		virtual void Paint(int x, int y, BOOL hasFocus);
		virtual void HandleEvent(GEVENT *event);
		void PaintMaskLayer(int x, int y, BOOL hasFocus);
		void PaintIconsLayer(int x, int y, BOOL hasFocus);
		void PaintPreviewLayer(int x, int y, BOOL hasFocus);
		void PaintSelected(int x, int y, BOOL hasFocus);
		void PaintScroll(int x, int y, BOOL hasFocus);
		REGISTERED_FILE_TYPE* GetRegisteredType(SCANDIRENT* pFile);
		void DrawThumbnail(Rect* pRect, int flags, int size, char mode);
		void SetFilename(int nID, char where);
		void SetRows(int num);
		void SetCols(int num);
		void CountSize();
		void ClipStart();
};

FilePick::FilePick(int left, int top, int width, int height, FILEBROWSER_PREFS* pPrefs) : Container(left, top, width, height)
{
	BYTE* pTile;

	nCursor 	= 0;		nCursorOld	= -1;
	nStart		= 0;		scrollSize	= 16;
	value 		= -1;		numfiles	= 0;
	
	scrlTotRows	= 1;		scrollerHg	= 0;
	scollAreaHg	= 0;		canFocus 	= 1;
	gotThumbL 	= 0; 		gotThumbS 	= 0;

	files 		= NULL;
	pCache 		= NULL;
	pNameInput	= NULL;
		
	showThumbs		= pPrefs->thumbnails;
	stretchPreview	= pPrefs->previewStretch;
	if (!pPrefs->previewArea) largePreviewSize = 0;
	else largePreviewSize = perc2val(width-scrollSize, 40);

	SetCols(pPrefs->cols);
	SetRows(pPrefs->rows);

	nThumbDrawTile = tileGetBlank();
	dassert(nThumbDrawTile >= 0);
	pTile = tileAllocTile(nThumbDrawTile, 320, 200);
	dassert(pTile != NULL);

	// use QBITMAP because setviewtotile does not working properly when already in tile
	pMaskLayer = (QBITMAP*)malloc(sizeof(QBITMAP)+(bytesperline*height));
	dassert(pMaskLayer != NULL);

	memset(pMaskLayer, 0, sizeof(QBITMAP));
	pMaskLayer->type	= kQBitmapRAW;
	pMaskLayer->bpl		= bytesperline;
	pMaskLayer->width	= width;
	pMaskLayer->height	= height;

	pIcoLayer = (QBITMAP*)malloc(sizeof(QBITMAP)+(bytesperline*height));
	dassert(pIcoLayer != NULL);

	memset(pIcoLayer, 0, sizeof(QBITMAP));
	pIcoLayer->type		= kQBitmapTRAW;
	pIcoLayer->bpl		= bytesperline;
	pIcoLayer->tcolor	= clr2std(kColorMagenta);
	pIcoLayer->width	= width-scrollSize-largePreviewSize;
	pIcoLayer->height	= height;

	if (largePreviewSize)
	{
		pPreviewLayer = (QBITMAP*)malloc(sizeof(QBITMAP)+(bytesperline*height));
		dassert(pPreviewLayer != NULL);

		memset(pPreviewLayer, 0, sizeof(QBITMAP));
		pPreviewLayer->type		= kQBitmapTRAW;
		pPreviewLayer->bpl		= bytesperline;
		pPreviewLayer->tcolor	= clr2std(kColorMagenta);
		pPreviewLayer->width	= largePreviewSize;
		pPreviewLayer->height	= height;
	}
	else
	{
		pPreviewLayer = NULL;
	}

	updateIcons = 1;
	CountSize();
}

FilePick::~FilePick()
{
	if (nThumbDrawTile >= 0)
		tileFreeTile(nThumbDrawTile);

	if (pIcoLayer)
		free(pIcoLayer);

	if (pMaskLayer)
		free(pMaskLayer);
	
	if (pPreviewLayer)
		free(pPreviewLayer);
	
	// save options
	gDirBroPrefs.rows 			= (autoRows) ? 0 : nRows;
	gDirBroPrefs.cols 			= (autoCols) ? 0 : nCols;
	gDirBroPrefs.previewArea 	= (largePreviewSize > 0);
	gDirBroPrefs.previewStretch	= stretchPreview;
	gDirBroPrefs.thumbnails 	= showThumbs;
	
}

void FilePick::DrawThumbnail(Rect* pRect, int flags, int size, char mode)
{
	tileDrawTileRect(pRect, flags, nThumbDrawTile, size, 0, 0, (mode == 2) ? 0 : 0x02);
}

void FilePick::CountSize()
{
	int wh = width-scrollSize-largePreviewSize;
	int hg = height;
	int seeRows;

	colWh = wh / nCols;
	colHg = hg / nRows;

	tmbSizeSWide = perc2val((colWh > colHg) ? colHg : colWh, 80);
	tmbSizeSTall = perc2val((colWh < colHg) ? colWh : colHg, 80);

	if (tmbSizeSWide % 2)
		tmbSizeSWide--;

	if (tmbSizeSTall % 2)
		tmbSizeSTall--;

	if (largePreviewSize)
	{
		if (largePreviewSize % 2)
			largePreviewSize++;

		Rect* pRect = new Rect(left+wh, top, left+wh+largePreviewSize-1, top+hg);
		tmbSizeLWide = perc2val((pRect->width() > pRect->height()) ? pRect->height() : pRect->width(), 90);
		tmbSizeLTall = perc2val((pRect->width() < pRect->height()) ? pRect->width() : pRect->height(), 90);

		if (tmbSizeLWide % 2)
			tmbSizeLWide--;

		if (tmbSizeLTall % 2)
			tmbSizeLTall--;
	}

	icoSize = perc2val((colWh < colHg) ? colWh : colHg, 40);
	namePad = perc2val((colWh < colHg) ? colWh : colHg, 8);

	if (icoSize % 2)
		icoSize--;

	scrlTotRows = ClipLow(ClipLow(numfiles - 1, 0) / nCols, 1);
	scollAreaHg = height - (scrollSize << 1);
	seeRows		= ClipHigh(nRows, scrlTotRows);
	scrollerHg	= ClipLow(perc2val(scollAreaHg, IVAL2PERC(seeRows, scrlTotRows)), scrollSize>>1);

	PaintMaskLayer(0, 0, 0);
}

void FilePick::PaintMaskLayer(int x, int y, BOOL hasFocus)
{
	int wh = width, hg = height, dx = x, dy = y;
	int colWhRem, colHgRem, i, j, mx, my;
	intptr_t bframe = frameplace;
	Rect* pRect; QBITMAP* pPic;

	gfxBackupClip();
	gfxSetClip(dx, dy, dx+wh, dy+hg);

	frameplace = (intptr_t)pMaskLayer->data;
	gfxSetColor(clr2std(25));
	gfxFillBox(dx, dy, dx+wh, dy+hg);

	if (largePreviewSize)
	{
		pRect = new Rect(dx, dy, dx+largePreviewSize-1, dy+hg);

		// draw large preview area
		/////////////////////////////
		gfxSetColor(clr2std(26));
		gfxLine(pRect->x0, pRect->y0, pRect->x1, pRect->y1);
		gfxLine(pRect->x0, pRect->y1, pRect->x1, pRect->y0);
		gfxVLine(pRect->x1, pRect->y0, pRect->y1);
	}

	wh = width-scrollSize-largePreviewSize;
	hg = height;

	// draw icons area
	/////////////////////////////
	colHgRem = hg % nRows;
	for (i = 0; i < nRows; i++)
	{
		dx = x+largePreviewSize;
		if (rngok(i, 1, nRows))
		{
			gfxSetColor(clr2std(26));
			gfxHLine(dy, dx, dx+wh-1);
		}

		colWhRem = wh % nCols;
		for (j = 0; j < nCols; j++)
		{
			if (rngok(j, 1, nCols))
			{
				gfxSetColor(clr2std(26));
				gfxVLine(dx, dy, dy + colHg);
			}

			dx+=colWh;
			if (--colWhRem >= 0)
				dx++;
		}

		dy+=colHg;
		if (--colHgRem >= 0)
			dy++;
	}

	if (scrollSize)
	{
		dx = x+wh+largePreviewSize;
		dy = y;

		// scoll area
		///////////////////////////////
		gfxSetColor(clr2std(22));
		gfxFillBox(dx, dy, dx+scrollSize, dy+hg);

		gfxSetColor(clr2std(26));
		gfxVLine(dx, y, y+hg);

		// buttons
		///////////////////////////////
		pPic = pBitmaps[6]; gfxSetColor(clr2std(20));
		pRect = new Rect(dx+1, dy, dx+scrollSize-1, dy+scrollSize);
		gfxFillBox(pRect);
		gfxRect(pRect);

		mx = pRect->x0+((pRect->width()>>1)-(pPic->width>>1));
		my = pRect->y0+((pRect->height()>>1)-(pPic->height>>1));
		gfxDrawBitmap(pPic, mx, my);

		pPic = pBitmaps[7];
		pRect = new Rect(dx+1, dy+hg-scrollSize, dx+scrollSize-1, dy+hg);
		gfxFillBox(pRect);

		mx = pRect->x0+((pRect->width()>>1)-(pPic->width>>1));
		my = pRect->y0+((pRect->height()>>1)-(pPic->height>>1));
		gfxDrawBitmap(pPic, mx, my);
	}

	frameplace = bframe;
	gfxRestoreClip();
}

void FilePick::PaintIconsLayer(int x, int y, BOOL hasFocus)
{
	int maxLines = ClipLow((colHg / qFonts[0]->height)>>1, 2);
	int wh = width-scrollSize-largePreviewSize, hg = height;
	int dx = x, dy = y, colWhRem, colHgRem, i, j, k, t;
	int cp = ClipLow(namePad, 4);
	intptr_t bframe = frameplace;
	char typeFc, typeBc;
	char isDir;
	
	REGISTERED_FILE_TYPE *pReg;
	QFONT* pFont; Rect* pRect;
	SCANDIRENT* pFile;
	char* p;

	gfxBackupClip();
	gfxSetClip(dx, dy, dx+wh, dy+hg);

	// draw icons and thumbnails
	/////////////////////////////
	frameplace = (intptr_t)pIcoLayer->data;
	memset((void*)frameplace, 255, bytesperline*hg);
	
	colHgRem = hg % nRows;
	for (i = 0, k = nStart; i < nRows; i++)
	{
		dx = x;
		colWhRem = wh % nCols;
		for (j = 0; j < nCols; j++, k++)
		{
			if (k >= numfiles)
			{
				frameplace = bframe;
				gfxRestoreClip();
				return;
			}

			pRect = new Rect(dx, dy, dx+colWh, dy+colHg);
			typeFc = clr2std(kColorWhite);
			typeBc = clr2std(kColorGreen);
			pFile = &files[k];

			if (pFile->flags & kDirExpDirMask)
			{
				if (pFile->flags & kDirExpDir) tileDrawTileRect(pRect, ALG_MIDDLE|ALG_CENTER, gSysTiles.icoFolder1, icoSize, 0, 0);
				else tileDrawTileRect(pRect, ALG_MIDDLE|ALG_CENTER, gSysTiles.icoDrive1, icoSize, 0, 0);
				pFont = qFonts[0];
				isDir = 1;
			}
			else
			{
				pFont = qFonts[1]; gotThumbS = 0; isDir = 0;
				if ((pReg = GetRegisteredType(pFile)) != NULL)
				{
					if (showThumbs && !(pFile->flags & kDirExpNoThumb) && pReg->pThumbFunc && (pReg->thumbType & kThumbSmall))
					{
						if (pReg->thumbType & kThumbCache)
						{
							if ((gotThumbS = pCache->Get((uint32_t)pFile->crc, nThumbDrawTile)) == 0)
							{
								if ((gotThumbS = pReg->pThumbFunc(pFile->full, nThumbDrawTile, pReg->thumbWidth, pReg->thumbHeight, pReg->thumbBg)) > 0)
								{
									pCache->Add((uint32_t)pFile->crc, nThumbDrawTile);
								}
								else
								{
									pFile->flags |= kDirExpNoThumb;
								}
							}
						}
						else if ((gotThumbS = pReg->pThumbFunc(pFile->full, nThumbDrawTile, pReg->thumbWidth, pReg->thumbHeight, pReg->thumbBg)) == 0)
						{
							pFile->flags |= kDirExpNoThumb;
						}
					}
					
					if (pReg->typeColor >= 0)	typeFc = clr2std(pReg->typeColor);
					if (pReg->typeBgColor >= 0) typeBc = clr2std(pReg->typeBgColor);
				}

				if (gotThumbS)
				{
					if (tilesizx[nThumbDrawTile] > tilesizy[nThumbDrawTile])
					{
						t = tmbSizeSWide;
					}
					else
					{
						t = tmbSizeSTall;
					}

					DrawThumbnail(pRect, ALG_MIDDLE|ALG_CENTER, t, gotThumbS);
				}
				else
				{
					tileDrawTileRect(pRect, ALG_MIDDLE|ALG_CENTER, gSysTiles.icoFile1, icoSize, 0, 0);
				}
			}
			
			pRect->scale(-namePad, -namePad);
			if (!isDir && (p = strrchr(pFile->name, '.')) != NULL && p != pFile->name) t = *p, *p = '\0'; else p = NULL;
			gfxDrawTextRect(pRect, ALG_BOTTOM|ALG_CENTER|kTextShadow, clr2std(kColorYellow), pFile->name, pFont, maxLines);
			if (p) *p = t;

			gfxDrawCaption(dx+cp, dy+cp, typeFc, typeBc, 2, pFile->type, qFonts[3]);

			dx+=colWh;
			if (--colWhRem >= 0)
				dx++;
		}

		dy+=colHg;
		if (--colHgRem >= 0)
			dy++;
	}

	frameplace = bframe;
	gfxRestoreClip();
	
}

void FilePick::PaintSelected(int x, int y, BOOL hasFocus)
{
	int wh = width-scrollSize-largePreviewSize, hg = height;
	int dx, dy = y, colWhRem, colHgRem, i, j, k;
	SCANDIRENT* pFile;
	Rect* pRect;

	// draw hover for focused and selected items
	////////////////////////////////////////////

	colHgRem = hg % nRows;
	for (i = 0, k = nStart; i < nRows; i++)
	{
		dx = x+largePreviewSize;
		colWhRem = wh % nCols;
		for (j = 0; j < nCols; j++, k++)
		{
			if (k >= numfiles)
				return;

			pRect = new Rect(dx+1, dy+1, dx+colWh-1, dy+colHg-1);
			pFile = &files[k];
			
			
			if (k == nCursor)
			{
				if (hasFocus)
				{
					gfxSetColor(clr2std(kColorCyan));
					gfxFillBox(pRect);
				}
				else
				{
					gfxSetColor(clr2std(24));
					gfxFillBox(pRect);
				}
				
				gfxSetColor(clr2std(20));
				gfxRect(pRect);
			}

			if (pFile->flags & kDirExpSelected)
			{
				gfxSetColor(clr2std(kColorLightBlue));
				pRect->scale(-3, -3);
				gfxFillBox(pRect);

				if (hasFocus)
				{
					gfxSetColor(clr2std(26));
					pRect->scale(-3, -3);
					gfxRect(pRect);
				}
			}

			dx+=colWh;
			if (--colWhRem >= 0)
				dx++;
		}

		dy+=colHg;
		if (--colHgRem >= 0)
			dy++;
	}
}

void FilePick::PaintPreviewLayer(int x, int y, BOOL hasFocus)
{
	Rect* pRect = new Rect(x, y, x+largePreviewSize-1, y+height);
	REGISTERED_FILE_TYPE* pReg = NULL;
	intptr_t bframe = frameplace;
	SCANDIRENT* pFile = NULL;
	char buf[256] = "\0";
	int t, hg = height;
	int twh, thg;

	// draw large preview area
	/////////////////////////////
	frameplace = (intptr_t)pPreviewLayer->data;
	memset((void*)frameplace, 255, bytesperline*hg);

	sprintf(buf, "Stretch: %s", onOff(stretchPreview));
	gfxDrawTextRect(pRect, ALG_TOP|ALG_CENTER|kTextShadow, clr2std(16), buf, qFonts[1]);
	buf[0] = '\0';

	if (rngok(nCursor, 0, numfiles))
	{
		pFile = &files[nCursor];
		pReg = GetRegisteredType(pFile);

		gotThumbL = 0; nCursorOld = nCursor;
		if (pReg && !(pFile->flags & kDirExpNoThumb) && pReg->pThumbFunc && (pReg->thumbType & kThumbLarge))
		{
			if (pReg->thumbType & kThumbCache)
			{
				if (!pCache->Get((uint32_t)pFile->crc, nThumbDrawTile))
				{
					if ((gotThumbL = pReg->pThumbFunc(pFile->full, nThumbDrawTile, pReg->thumbWidth, pReg->thumbHeight, pReg->thumbBg)) > 0)
					{
						pCache->Add((uint32_t)pFile->crc, nThumbDrawTile);
					}
					else
					{
						pFile->flags |= kDirExpNoThumb;
					}
				}
				else
				{
					gotThumbL = 1;
				}
			}
			else if ((gotThumbL = pReg->pThumbFunc(pFile->full, nThumbDrawTile, pReg->thumbWidth, pReg->thumbHeight, pReg->thumbBg)) == 0)
			{
				pFile->flags |= kDirExpNoThumb;
			}
		}
	}

	if (gotThumbL)
	{
		twh = tilesizx[nThumbDrawTile];
		thg = tilesizy[nThumbDrawTile];
		
		t = (twh > thg) ? tmbSizeLWide : tmbSizeLTall;
		if (!stretchPreview && !(pReg->thumbType & kThumbForceStretch) && twh < t && thg < t)
			t = (twh < thg) ? thg : twh; // 1:1 size

		DrawThumbnail(pRect, ALG_MIDDLE|ALG_CENTER, t, gotThumbL);		
	}
	else
	{
		gfxDrawTextRect(pRect, ALG_MIDDLE|ALG_CENTER|kTextShadow, clr2std(16), "NO PREVIEW", qFonts[2]);
	}
		
	if (pReg && !isempty(pReg->description))
	{
		sprintf(buf, "%s (%s)", pReg->description, pFile->type);
	}
	else if (pFile)
	{
		if (pFile->flags & kDirExpDir)			strcpy(buf, "Directory");
		else if (pFile->flags & kDirExpDisk)	strcpy(buf, "Drive");
		else
		{
			if (!isempty(pFile->type))
				strcpy(buf, pFile->type);

			strcat(buf, " file");
		}
	}

	if (buf[0] != '\0')
	{
		int wp = perc2val(largePreviewSize, 10);
		int hp = ClipLow(perc2val(height, 5), pFont->height<<1);
		pRect = new Rect(x+wp, y+height-(hp<<1), x+largePreviewSize-wp, y+height-hp);
		gfxDrawTextRect(pRect, ALG_TOP|ALG_CENTER|kTextShadow, clr2std(16), buf, qFonts[1]);
	}
	
	frameplace = bframe;
	
}

void FilePick::PaintScroll(int x, int y, BOOL hasFocus)
{
	// draw scroller
	///////////////////////////////
	gfxSetColor(clr2std((hasFocus) ? 20 : 21));
	int n = perc2val(scollAreaHg-scrollerHg, IVAL2PERC(nCursor / nCols, scrlTotRows));
	gfxFillBox(x+1, y+scrollSize+n, x+scrollSize-1, y+scrollSize+scrollerHg+n);
}

REGISTERED_FILE_TYPE* FilePick::GetRegisteredType(SCANDIRENT* pFile)
{
	if (rngok(pFile->extra, 0, LENGTH(gRegTypes)))
		return &gRegTypes[pFile->extra];

	return NULL;
}

void FilePick::Paint( int x, int y, BOOL hasFocus )
{	
	gfxDrawBitmap(pMaskLayer, x, y);	// background
	PaintSelected(x, y, hasFocus);		// selected or focused items

	if (scrollSize) // show scrollbar
		PaintScroll(x+width-scrollSize, y, hasFocus);

	if (updateIcons)
	{
		PaintIconsLayer(0, 0, hasFocus);	// update info for items (icons, small thumbnails, names)
		nCursorOld = -1;					// this forces update of large thumbnail as well
		updateIcons = 0;
	}

	if (largePreviewSize)
	{
		if (nCursor != nCursorOld)
			PaintPreviewLayer(0, 0, hasFocus);

		// overlay large thumbnail
		gfxDrawBitmap(pPreviewLayer, x, y);
	}

	// overlay items info
	gfxDrawBitmap(pIcoLayer, x+largePreviewSize, y);
}

void FilePick::HandleEvent( GEVENT *event )
{
	if (event->type & evMouse)
	{
		if (event->type == evMouseDown || event->mouse.wheel)
		{
			int x1 = left+largePreviewSize;	int x2 = x1+(width-largePreviewSize-scrollSize);
			int y1 = top;					int y2 = y1+height;
			
			int nFiles = ClipLow(numfiles - 1, 0);
			int nCol, nRow, t;

			if (rngok(event->mouse.x, x1, x2) && rngok(event->mouse.y, y1, y2))
			{
				x1 = event->mouse.x-largePreviewSize;
				y1 = event->mouse.y;

				if (event->type == evMouseDown)
				{
					nCol = ClipRange(x1/colWh, 0, nCols-1); nRow = ClipRange(y1/colHg, 0, nRows-1);
					nCursor = ClipRange((nRow*nCols) + nStart + nCol, nStart, nFiles);
					value = nCursor + mrUser;
					
					if (event->mouse.doubleClick && event->mouse.button == 0)
						EndModal(value);
				}
				else if (event->mouse.wheel)
				{
					t = nStart;
					if (event->mouse.wheel > 0)
					{
						if (nStart + nCols < ClipLow(nFiles - (nCols*nRows), 0))
							nStart += nCols;
						
						if (nCursor + nCols < nFiles)
							nCursor += nCols;
					}
					else
					{
						if (nStart - nCols >= 0)
							nStart -= nCols;
						
						if (nCursor - nCols >= 0)
							nCursor -= nCols;
					}
					
					ClipStart();
					value = nCursor + mrUser;
					updateIcons = (t != nStart);
				}
			}
			
			if (nCursor != nCursorOld)
				SetFilename(nCursor, 0x01);
		}
	}
	else if (event->type == evKeyDown)
	{
		BYTE key = event->key.make;

		switch (key)
		{
			case KEY_UP:
			case KEY_DOWN:
			case KEY_LEFT:
			case KEY_RIGHT:
			case KEY_HOME:
			case KEY_END:
			case KEY_PAGEUP:
			case KEY_PAGEDN:
				switch(key)
				{
					case KEY_UP:
						if (event->key.shift)
						{
							SetRows(nRows + 1);
							updateIcons = 1;
							CountSize();
						}
						else if (nCursor - nCols >= 0)
						{
							nCursor -= nCols;
						}
						break;
					case KEY_DOWN:
						if (event->key.shift)
						{
							if (nRows > 1)
							{
								SetRows(nRows - 1);
								updateIcons = 1;
								CountSize();
							}
						}
						else if (nCursor + nCols < numfiles)
						{
							nCursor += nCols;
						}
						break;
					case KEY_LEFT:
						if (event->key.shift)
						{
							SetCols(nCols + 1);
							updateIcons = 1;
							CountSize();
						}
						else if (nCursor - 1 >= 0)
						{
							nCursor--;
						}
						break;
					case KEY_RIGHT:
						if (event->key.shift)
						{
							if (nCols > 1)
							{
								SetCols(nCols - 1);
								updateIcons = 1;
								CountSize();
							}
						}
						else if (nCursor + 1 < numfiles)
						{
							nCursor++;
						}
						break;
					case KEY_PAGEUP:
						if (nCursor - (nRows*nCols) >= 0)
						{
							updateIcons = 1;
							nStart		= ClipLow(nStart-(nRows*nCols), 0);
							nCursor		-= (nRows*nCols);
						}
						break;
					case KEY_PAGEDN:
						if (nCursor + (nRows*nCols) < numfiles)
						{
							updateIcons = 1;
							nCursor 	+= (nRows*nCols);
							nStart  	+= (nRows*nCols);
						}
						break;
					case KEY_HOME:
						nCursor = 0;
						break;
					case KEY_END:
						nCursor = ClipLow(numfiles-1, 0);
						break;
				}

				if (!updateIcons)
					updateIcons = !rngok(nCursor, nStart, nStart + (nCols*nRows));

				SetFilename(nCursor, 0x01);
				nCursorOld = -1;
				event->Clear();
				ClipStart();
				break;
			case KEY_SPACE:
			case KEY_BACKSPACE:
			case KEY_DELETE:
			case KEY_INSERT:
				switch(key)
				{
					case KEY_BACKSPACE:		value = -1;		break;	// move up one level
					case KEY_DELETE:		value = -2;		break;	// delete item
					case KEY_INSERT:		value = -3;		break;	// create dir
					case KEY_SPACE:			value = -4;		break;	// rename
				}
				EndModal(value);
				event->Clear();
				break;
			case KEY_F2:
				showThumbs = !showThumbs;
				SetCols(nCols); SetRows(nRows);
				updateIcons = 1;
				event->Clear();
				break;
			case KEY_F3:
				stretchPreview = !stretchPreview;
				nCursorOld = -1;
				event->Clear();
				break;
			case KEY_F5: // clear cache / rescan
				value = (event->key.control) ? -6 : -5;
				event->Clear();
				EndModal(value);
				break;
			case KEY_F10:
				SetCols(0); SetRows(0);
				updateIcons = 1;
				event->Clear();
				CountSize();
				break;
		}
	}
}

void FilePick::ClipStart()
{
	while (nCursor < nStart)
		nStart -= nCols;
	
	nStart = ClipLow(nStart, 0);
	while (nStart + nRows * nCols <= nCursor)
		nStart += nCols;
}

void FilePick::SetRows(int num)
{
	int minSize = (showThumbs) ? 48 : 32;
	
	if (num <= 0)
	{
		nRows = 3 + (height / 200);
		if (tallscreen)
			nRows++;

		autoRows = 1;
	}
	else if (height / num >= minSize)
	{
		if (num < 16)
		{
			nRows = num;
			autoRows = 0;
		}
	}
	else
	{
		SetRows(0);
	}
}

void FilePick::SetCols(int num)
{
	int minSize = (showThumbs) ? 48 : 32;
	
	if (num <= 0)
	{
		nCols = 3 + (width / 320);
		if (widescreen)
			nCols++;

		autoCols = 1;
	}
	else if ((width-scrollSize-largePreviewSize) / num >= minSize)
	{
		if (num < 16)
		{
			nCols = num;
			autoCols = 0;
		}
	}
	else
	{
		SetCols(0);
	}
}

void FilePick::SetFilename(int nID, char where)
{
	if (rngok(nID, 0, numfiles))
	{
		char name[BMAX_PATH];
		SCANDIRENT* pFile = &files[nID];
		
		if (pFile->flags & kDirExpDirMask)
			strcpy(name, pFile->name);	
		else
			getFilename(pFile->name, name, 0);
		
		if (where & 0x01)
		{
			strcpy(pNameInput->string, name);
			pNameInput->len = strlen(name);
			pNameInput->pos = pNameInput->len;
		}
		
		if (where & 0x02)
		{
			strcpy(pNameInput->placeholder, name);
		}
	}
	else
	{
		if (where & 0x01)
		{
			pNameInput->string[0] = '\0';
			pNameInput->len = 0;
			pNameInput->pos = 0;
		}
		
		if (where & 0x02)
		{
			pNameInput->placeholder[0] = '\0';
		}
	}
}

/** File browser dialog
********************************************************************************/
class FileBrowser
{
	private:
		//-------------------------------------------------------------
		static ThumbCache* thumbCache;
		static int numfiles, numdirs;
		static char path[BMAX_PATH];
		static SCANDIRENT* files;
		//-------------------------------------------------------------
		char DirScan(char* pPath, char* pFilter);
		char DirCreateDlg(char* pWhere, char* out);
		char DirChange(SCANDIRENT* pDir);
		int  DirCountSubLevel(void);
		//-------------------------------------------------------------
		int ItemFindID(char* pNeedle);
		SCANDIRENT* ItemFind(char* pNeedle);
		RESHANDLE ItemFind_RFF(char* pNeedle, char* pFilter);
		char ItemRemoveDlg(char* pNeedle);
		char ItemRenameDlg(char* pNeedle, char* out);
		//-------------------------------------------------------------
		void WindowFormatTitle(char* newTitle, Window* pWindow);
		void ButtonEnable(BitButton2* pButton, QBITMAP* pPic);
		void ButtonDisable(BitButton2* pButton, QBITMAP* pPic);
		void ButtonEnable(TextButton* pButton, char fontColor);
		void ButtonDisable(TextButton* pButton);
		void ConcatToPath(char* pName);
		void SelectAllFiles(void);
		void UnselectAllFiles(void);
		char TypeNameDlg(char* pTitle, char* pNeedle, char* out, char emptyName);
		char IsNameCorrect(char* pName, char allowDot = 0);
		char ChangeTypeDlg(char* pNeedle, char* pFilter);
		void SanitizePath(char* pPath);
		//-------------------------------------------------------------
		inline char IsRoot(char* pPath) { return isempty(pPath); }
	public:
		char* ShowDialog(char* pPath, char* pFileName, char* pFilter, char* pTitle, int flags);
		friend char* dirBrowse(char* pAPath, char* pFilter, char* pTitle, int flags);
		friend char dirBrowseEnumItems(int* idx, char* out, int which);
		friend int dirBrowseCountItems(int which);
};

ThumbCache* FileBrowser::thumbCache	= new ThumbCache(kThumbnailCacheFile, CACHE_PRM);
char FileBrowser::path[BMAX_PATH]	= "\0";
SCANDIRENT* FileBrowser::files		= NULL;
int FileBrowser::numdirs			= 0;
int FileBrowser::numfiles			= 0;

char FileBrowser::DirScan(char* pPath, char* pFilter)
{
	struct
	{
		time_t mtime;
		char path[BMAX_PATH];
	}
	cacheid;
	
	SCANDIRENT *pFile;
	int i, k, t;
	char* ext;

	if (files)
		free(files);
	
	numfiles = numdirs = 0;
	
	// try get system drives
	if (IsRoot(pPath) && (files = driveScan(&numdirs)) != NULL)
	{
		for (i = 0; i < numdirs; i++)
		{
			pFile = &files[i];
			
			pFile->extra = -1;
			pFile->flags = kDirExpDisk; // make flags compatible
			strcpy(pFile->type, "DRV");
		}
		
		if (numdirs)
			return 1;
		
		// well, fall to the cwd otherwise...
		free(files);
	}
	
	// try get contents of the directory
	if ((files = dirScan(pPath, pFilter, &numfiles, &numdirs, 0x01)) != NULL)
	{
		t = numfiles+numdirs;
		for (i = 0; i < t; i++)
		{
			pFile = &files[i];
			pFile->extra = -1;
			if (pFile->flags & BS_IFDIR)
			{
				pFile->flags = kDirExpDir; // make flags compatible
				strcpy(pFile->type, "DIR");
			}
			else
			{
				k = LENGTH(gRegTypes);
				while(--k >= 0)
				{
					ext = (char*)gRegTypes[k].type;
					if (ext[0] == '.') ext =& ext[1];
					if (stricmp(ext, pFile->type) == 0)
						break;
				}

				pFile->flags = kDirExpFile; // make flags compatible
				pFile->extra = k;			// store index of registered file type here
				strupr(pFile->type);
			}
			
			SanitizePath(pFile->full);
			cacheid.mtime = pFile->mtime; strcpy(cacheid.path, pFile->full);
			pFile->crc = crc32once((unsigned char*)&cacheid, strlen(cacheid.path)+sizeof(cacheid.mtime));
		}
		
		qsort(&files[0], t, 				sizeof(SCANDIRENT), (int(*)(const void*,const void*))qsSortDirMoveUp);	// dirs on top
		qsort(&files[0], numdirs,			sizeof(SCANDIRENT), (int(*)(const void*,const void*))qsSortByName);		// ASCII order
		qsort(&files[numdirs], numfiles,	sizeof(SCANDIRENT),	(int(*)(const void*,const void*))qsSortByName);		// ASCII order
		return 1;
	}
	
	return 0;
}

char FileBrowser::DirChange(SCANDIRENT* pDir)
{
	int i = strlen(path);

	if (pDir)
	{
		if (i && !slash(path[i-1]))
			path[i++] = '/';
		
		strcpy(&path[i], pDir->name);
		return 1;
	}
	else if (DirCountSubLevel() > 0)
	{
		while(--i > 0 && !slash(path[i]));
		path[i] = '\0';
		return 1;
	}

	return 0;
}

char FileBrowser::DirCreateDlg(char* pWhere, char* out)
{
	char newpath[BMAX_PATH];
	if (TypeNameDlg("Create new folder:", pWhere, newpath, 1))
	{
		if (mkdir(newpath) == 0)
		{
			chmod(newpath, S_IREAD|S_IWRITE);
			getFilename(newpath, out);
			return 1;
		}
		
		Alert("Could not create \"%s\" (errno: %d)", newpath, errno);
	}
	
	return 0;
}

int FileBrowser::DirCountSubLevel(void)
{
	int i = strlen(path);
	int c = 0;

	if (i > 1)
	{
		c = 1;
		while(--i >= 0)
		{
			if (slash(path[i]))
				c++;
		}
	}

	return c;
}

char FileBrowser::IsNameCorrect(char* pName, char allowDot)
{
	int i = 0;
	while(pName[i])
	{
		switch(pName[i])
		{
			case '.':
				if (allowDot) break;
			case '/':		case '\\':
			case ':':		case '?':
			case ';':		case '%':
			case '*':		case '$':
			case '|':		case '"':
			case '\'':		case ',':
				return 0;
		}
		
		i++;
	}

	return (i > 0);
}

int FileBrowser::ItemFindID(char* pNeedle)
{
	char *name, buf[BMAX_PATH], needle[BMAX_PATH];
	int i = numfiles+numdirs;
	SCANDIRENT* pFile;

	strcpy(needle, pNeedle);
	removeExtension(needle);

	while(--i >= 0)
	{
		pFile = &files[i];
		strcpy(buf, pFile->name); removeExtension(buf);
		if (stricmp(buf, needle) == 0)
			return i;
	}

	return -1;
}

SCANDIRENT* FileBrowser::ItemFind(char* pNeedle)
{
	int nID;
	if ((nID = ItemFindID(pNeedle)) >= 0)
		return &files[nID];

	return NULL;
}

RESHANDLE FileBrowser::ItemFind_RFF(char* pNeedle, char* pFilter)
{
	RESHANDLE hRes = NULL;
	char needle[BMAX_PATH]; strcpy(needle, pNeedle);
	ChangeTypeDlg(needle, pFilter);
	fileExists(needle, &hRes);
	return hRes;
}

char FileBrowser::ItemRenameDlg(char* pNeedle, char* out)
{
	char newpath[BMAX_PATH];
	if (TypeNameDlg("Type new name:", pNeedle, newpath, 0))
	{
		if (chmod(pNeedle, S_IREAD|S_IWRITE) == 0 && rename(pNeedle, newpath) == 0)
		{
			if (out)
				getFilename(newpath, out, 1);
			
			return 1;
		}
		
		Alert("Could not rename \"%s\" to \"%s\" (errno: %d)", pNeedle, newpath, errno);
	}
	
	return 0;
}

char FileBrowser::ItemRemoveDlg(char* pNeedle)
{
	char newpath[BMAX_PATH] = "\0";
	char cwd[BMAX_PATH];
	int r = -1, t;
	
	if ((t = sprintf(newpath, path)) > 0 && !slash(newpath[t-1]))
		catslash(newpath), t++;
	
	strcpy(&newpath[t], pNeedle);
	if (!fileExists(newpath))
		return 1;
	
	if (isDir(newpath))
	{
		if (Confirm("Remove folder \"%s\" and all it's contents?", pNeedle))
		{
			getcwd(cwd, sizeof(cwd));
			r = (dirRemoveRecursive(newpath) > 0);
			chdir(cwd);
		}
	}
	else
	{
		r = (chmod(newpath, S_IREAD|S_IWRITE) == 0 && unlink(newpath) == 0);
	}
	
	if (r == 0)
		Alert("Could not remove \"%s\" (errno: %d).", pNeedle, errno);
	
	return (r > 0);
}

char FileBrowser::ChangeTypeDlg(char* pNeedle, char* pFilter)
{
	char *string, *pStr, *p; NAMED_TYPE* types = NULL;
	char ext[BMAX_PATH] = "\0", val[BMAX_PATH];
	int t = 1, i = 0;
	
	if (isempty(pFilter))
		return 1;
	
	getFiletype(pNeedle, ext, 0);
	string = (char*)malloc(strlen(pFilter)+1);	
	dassert(string != NULL);
	pStr = string;
	
	// list all the allowed extensions
	while((p = enumStrGetChar(t++, val, pFilter, '.', NULL)) != NULL)
	{
		// found matching, no need to continue (file already have it)
		if (!isempty(ext) && stricmp(ext, val) == 0)
			break;
		
		types = (NAMED_TYPE*)realloc(types, sizeof(NAMED_TYPE)*(i+1));
		dassert(types != NULL);
		
		types[i].name = pStr, pStr+=(sprintf(pStr, "%s", val)+1);
		types[i].id = i++;
	}
	
	if (types && p == NULL) // didn't found matching
	{
		if (i > 1)
		{
			// user must select from 2 or more extensions
			while((t = showButtons(types, i, "Pick file type")) < mrUser);
			i = t - mrUser;
		}
		else
		{
			// auto set the only possible exension
			i = 0;
		}
		
		ChangeExtension(pNeedle, types[i].name);
	}
	
	if (types)
		free(types);
	
	free(string);
	return 1;
}

char* FileBrowser::ShowDialog(char* pPath, char* pFileName, char* pFilter, char* pTitle, int flags)
{
	const int kStatHeight		= 16;	const int kToolsHeight		= 42;
	const int kWindowSize		= 75;	const int kButH				= 30;
	const int pad				= 4;
	
	char* p, DLGUPD = 1;
	char selMulti	  = ((flags & kDirExpMulti) != 0);
	char selForSaving = ((flags & kDirExpSave) != 0);
	char buff[256] = "\0", prvDir[BMAX_PATH] = "\0";
	int nCode=0, nFile, dwh, dhg;
	int i;
	
	Panel *pPickerP, *pToolsP, *pStatP; Label* pUpL, *pStatL;
	TextButton *pSelB, *pSelAllB, *pSelNoneB;
	FilePick *pPicker; EditText *pNameE;
	BitButton2 *pUpB; Container* pFocus;
	SCANDIRENT *pFile; RESHANDLE hRes;
	QBITMAP* pPic;
	
	strcpy(path, pPath);
	SanitizePath(path);
	if (isempty(pTitle))
		pTitle = ((selForSaving) ? "Save as" : (selMulti) ? "Select files" : "Open file");

	// create the dialog
	//////////////////////////////////////
	Window dialog(0, 0, ClipLow(perc2val(kWindowSize, xdim), 320), ClipLow(perc2val(kWindowSize, ydim), 200), "");
	dwh = dialog.client->width;
	dhg = dialog.height;

	pPickerP			= new Panel(dialog.left, dialog.top, dialog.client->width, dialog.client->height-kToolsHeight-kStatHeight, 0, 0, 0);
	pToolsP				= new Panel(dialog.left, pPickerP->top+pPickerP->height, dialog.client->width, kToolsHeight, 1, 1, 0);
	pStatP				= new Panel(dialog.left, pToolsP->height+pPickerP->top+pPickerP->height, dialog.client->width, kStatHeight, 1, 1, 0);
	
	pPicker 			= new FilePick(0, 0, pPickerP->width, pPickerP->height, &gDirBroPrefs);
	
	pSelB 				= new TextButton(0, 0, (dwh <= 320) ? 60 : 100, kButH, (selForSaving) ? "Save" : "Open", mrOk);
	pSelB->top			= (pToolsP->height>>1)-(pSelB->height>>1);
	pSelB->left 		= pToolsP->width-pSelB->width-pad;

	pSelNoneB 			= new TextButton(0, 0, (dwh <= 320) ? 60 : 80, kButH, "None", -1006);
	pSelNoneB->top		= (pToolsP->height>>1)-(pSelNoneB->height>>1);
	pSelNoneB->left		= pToolsP->width-pSelB->width-pSelNoneB->width-pad;

	pSelAllB 			= new TextButton(0, 0, (dwh <= 320) ? 60 : 80, kButH, "All", -1005);
	pSelAllB->top		= (pToolsP->height>>1)-(pSelAllB->height>>1);
	pSelAllB->left		= pToolsP->width-pSelB->width-pSelNoneB->width-pSelAllB->width-pad;
	
	pPic				= pBitmaps[0];
	pUpB 				= new BitButton2(pad, 0, pPic->width+pad, pPic->height+pad, NULL, -1);
	pUpB->top			= (pToolsP->height>>1)-(pUpB->height>>1);
	pUpB->left			= pad;

	pUpL				= new Label(0, 0, "Move up");
	pUpL->top			= (pToolsP->height>>1)-(pUpL->height>>1);
	pUpL->left			= pUpB->left+pUpB->width+pad;
	
	pStatL				= new Label(0, 0, buff);
	pStatL->fontColor	= 28;
	
	i = ClipHigh(perc2val(pSelB->left-(pUpL->left+pUpL->width), 90), pFont->width * 42);
	pNameE				= new EditText(pSelB->left-pad-i, 0, i, 22, "", 0x01);
	pNameE->top			= (pToolsP->height>>1)-(pNameE->height>>1);
	pNameE->maxlen		= 255;
	
	strcpy(pNameE->placeholder, "Type name...");
	
	if (!isempty(pFileName))
	{
		strcpy(buff, pFileName);
		removeExtension(buff);
	}
	
	pPicker->pCache		= thumbCache;
	pPicker->pNameInput	= pNameE;
	
	// insert necessary elements
	//////////////////////////////////////
	pPickerP->Insert(pPicker);
	pStatP->Insert(pStatL);
	
	pToolsP->Insert(pUpB);
	pToolsP->Insert(pUpL);
	
	if (selMulti)
	{
		pToolsP->Insert(pSelAllB);
		pToolsP->Insert(pSelNoneB);
		pToolsP->Insert(pSelB);
	}
	else
	{
		pToolsP->Insert(pNameE);
		pToolsP->Insert(pSelB);
	}
	
	dialog.Insert(pPickerP);
	dialog.Insert(pToolsP);
	dialog.Insert(pStatP);
	
	dialog.focus	= (selForSaving) ? ((Widget*)pNameE) : ((Widget*)pPicker);
	dialog.left		= (xdim-dialog.width) >>1;
	dialog.top		= (ydim-dialog.height)>>1;
	
	while( 1 )
	{
		pSelNoneB->pressed	= 0;
		pSelAllB->pressed	= 0;
		pSelB->pressed		= 0;
		pUpB->pressed		= 0;
		
		if (DLGUPD)
		{
			// dialog update requested
			WindowFormatTitle(pTitle, &dialog);
			DirScan(path, pFilter);

			if (!DirCountSubLevel())
			{
				ButtonDisable(pUpB, pBitmaps[13]);
				pUpL->fontColor	= kColorDarkGray;
			}
			else
			{
				ButtonEnable(pUpB, pBitmaps[0]);
				pUpL->fontColor	= 28;
			}

			if (!numfiles && !numdirs && !selForSaving)
			{
				ButtonDisable(pSelB);
			}
			else
			{
				ButtonEnable(pSelB, kColorBlue);
			}

			if (numfiles)
			{
				ButtonEnable(pSelAllB, kColorGreen);
				ButtonEnable(pSelNoneB, kColorRed);
			}
			else
			{
				ButtonDisable(pSelAllB);
				ButtonDisable(pSelNoneB);
			}

			pPicker->numfiles		= numfiles+numdirs;
			pPicker->files			= files;
			pPicker->nCursorOld		= -1;
			pPicker->updateIcons 	= 1;

			if (nCode == 0)			pPicker->nCursor = ClipLow(ItemFindID(buff), 0);
			else if (nCode == -1)	pPicker->nCursor = ClipLow(ItemFindID(prvDir), 0);
			else pPicker->nCursor = 0;

			pPicker->SetFilename(pPicker->nCursor, 0x01);
			pPicker->CountSize();
			pPicker->ClipStart();
			
			DLGUPD = 0;
			BeepOk();
		}
		
		p = pStatL->string;
		p+=sprintf(p, "%d folders and %d files found", numdirs, numfiles);
		if (selMulti)
			sprintf(p, " (%d selected)", dirBrowseCountSelected());

		pStatL->left = (pStatP->width>>1)-(gfxGetTextLen(pStatL->string, pFont)>>1);
		pStatL->top	 = (pStatP->height>>1)-(pFont->height>>1);

		nCode = ShowModal(&dialog, kModalNoCenter);
		if (nCode == mrCancel)
			break;
		
		if (pToolsP->focus != &pToolsP->head)			pFocus = (Container*)pToolsP->focus;
		else if (pPickerP->focus != &pPickerP->head)	pFocus = (Container*)pPickerP->focus;
		else											pFocus = NULL;
		
		dialog.ClearFocus();
		
		if (!IsRoot(path))
		{
			if (nCode == -1 || (pUpB == (BitButton2*)pFocus))
			{
				// move up one level
				getCurDir(path, prvDir);
				DLGUPD = DirChange(NULL);
				continue;
			}
			else if (nCode == -2)
			{
				// remove
				if (rngok(pPicker->nCursor, 0, pPicker->numfiles))
				{
					if ((DLGUPD = ItemRemoveDlg((char*)files[pPicker->nCursor].name)) > 0)
					{
						if (pPicker->nCursor + 1 < pPicker->numfiles) nFile = pPicker->nCursor + 1;
						else if (pPicker->nCursor - 1 >= 0) nFile = pPicker->nCursor - 1;
						else nFile = 0;
						
						if (rngok(nFile, 0, pPicker->numfiles))
						{
							// gonna find next or previous file on dialog update
							strcpy(buff, (const char*)files[nFile].name);
							nCode = 0;
						}
					}
				}
				
				continue;
			}
			else if (nCode == -3)
			{
				// new dir
				if ((DLGUPD = DirCreateDlg(path, buff)) > 0)
					nCode = 0;
				
				continue;
			}
			else if (nCode == -4)
			{
				// rename
				if (rngok(pPicker->nCursor, 0, pPicker->numfiles))
				{
					if ((DLGUPD = ItemRenameDlg((char*)files[pPicker->nCursor].full, buff)) > 0)
						nCode = 0;
				}
				
				continue;
			}
		}
		
		
		if (nCode == -5 || nCode == -6)
		{
			// clear cache / rescan dir
			if (nCode == -6)
				thumbCache->Erase();
			
			DLGUPD = 1;
			if (rngok(pPicker->nCursor, 0, pPicker->numfiles))
			{
				strcpy(buff, files[pPicker->nCursor].name);
				nCode = 0;
			}
		}
		else if (pSelAllB == (TextButton*)pFocus)		SelectAllFiles();
		else if (pSelNoneB == (TextButton*)pFocus)		UnselectAllFiles();
		else if (pPicker == (FilePick*)pFocus)
		{
			nFile = -1;
			if (nCode == mrOk) nFile = pPicker->nCursor;
			else if (pPicker->value >= mrUser)
				nFile = pPicker->value - mrUser;

			if (rngok(nFile, 0, pPicker->numfiles))
			{
				pFile = &files[nFile];
				if (pFile->flags & kDirExpDirMask) DLGUPD = DirChange(pFile);
				else if (selMulti) pFile->flags ^= kDirExpSelected;
				else
				{
					pFile->flags |= kDirExpSelected;	// mark it selected so enumSelected work
					ConcatToPath(pFile->name);
					return path;
				}
			}
		}
		else if ((pSelB == (TextButton*)pFocus) || (pNameE == (EditNumber*)pFocus))
		{
			p = pNameE->string;
			if (isempty(p) && rngok(pPicker->nCursor, 0, pPicker->numfiles))
				p = files[pPicker->nCursor].name;

			if (!isempty(p))
			{
				if ((pFile = ItemFind(p)) == NULL)
				{
					if (!selMulti && !IsRoot(path))
					{
						if (!selForSaving)
						{
							// try to search in the game RFF when allowed...
							if ((flags & kDirExpRff) && (hRes = ItemFind_RFF(p, pFilter)) != NULL)
							{
								sprintf(path, "%s.%s", hRes->name, hRes->type);
								return path;
							}
							
							Alert("File \"%s\" not found.", p);
						}
						else if (IsNameCorrect(p, 1))
						{
							strcpy(buff, p); ChangeTypeDlg(buff, pFilter);
							ConcatToPath(buff);
							return path;
						}
						else
						{
							Alert("\"%s\" is incorrect name.");
						}
					}
				}
				else if (pFile->flags & kDirExpDirMask)
				{
					DLGUPD = DirChange(pFile);
				}
				else
				{
					for (i = 0; i < pPicker->numfiles; i++)
					{
						if (files[i].flags & kDirExpSelected)
						{
							// return first selected file
							ConcatToPath(files[i].name);
							return path;
						}
					}
					
					pFile->flags |= kDirExpSelected; 	// mark it selected so enumSelected work
					ConcatToPath(pFile->name);
					return path;
				}
			}
		}
	}

	if (!isempty(pFileName))
		ConcatToPath(pFileName);
	
	return NULL;
}

void FileBrowser::WindowFormatTitle(char* newTitle, Window* pWindow)
{
	TitleBar* pBar = pWindow->titleBar;
	int len1, len2;
	char buf[256];

	if (IsRoot(path))
	{
		pBar->len = sprintf(pBar->string, "%s", newTitle);
		return;
	}

	len1 = len2 = sprintf(buf, "%s", path);
	while(len1 >= 0)
	{
		sprintf(pBar->string, "%s - %s", buf, newTitle);
		if (gfxGetTextLen(pBar->string, pFont) >= pWindow->width - 20)
		{
			buf[--len1] = 0;
			continue;
		}

		break;
	}

	if (len1 != len2)
		pBar->len = sprintf(pBar->string, "%s... - %s", buf, newTitle);
}

void FileBrowser::ButtonEnable(BitButton2* pButton, QBITMAP* pPic)
{
	pButton->pBitmap	= pPic;
	pButton->disabled  	= 0;
	pButton->canFocus  	= 1;
}

void FileBrowser::ButtonDisable(BitButton2* pButton, QBITMAP* pPic)
{
	pButton->pBitmap	= pPic;
	pButton->disabled 	= 1;
	pButton->canFocus 	= 0;
}

void FileBrowser::ButtonDisable(TextButton* pButton)
{
	pButton->disabled  = 1;
	pButton->canFocus  = 0;
	pButton->fontColor = kColorDarkGray;
}

void FileBrowser::ButtonEnable(TextButton* pButton, char fontColor)
{
	pButton->disabled  = 0;
	pButton->canFocus  = 1;
	pButton->fontColor = fontColor;
}

char FileBrowser::TypeNameDlg(char* pTitle, char* pNeedle, char* out, char emptyName)
{
	char tmp[BMAX_PATH], name[BMAX_PATH] = "\0", type[BMAX_PATH];
	char newpath[BMAX_PATH], firstTime = 1;
	int i, t;
	
	getFilename(pNeedle, tmp, 0); getFiletype(pNeedle, type, 1);
	if ((t = sprintf(newpath, path)) > 0 && !slash(newpath[t-1]))
		catslash(newpath), t++;
	
	while( 1 )
	{
		if (!emptyName) getFilename(pNeedle, name, 0); else name[0] = '\0';
		if (!GetStringBox(pTitle, name) || stricmp(name, tmp) == 0)
			break;
		
		if (IsNameCorrect(name))
		{
			strcpy(&newpath[t], name); strcat(newpath, type);
			getFilename(newpath, tmp, 1);
			
			i = 1;
			while(fileExists(newpath))
				sprintf(&newpath[t], "%s_%d%s", name, ++i, type);
			
			getFilename(newpath, name, 1);
			if (i != 1)
			{
				if ((i = YesNoCancel("\"%s\" already exists. Use name \"%s\" instead?", tmp, name)) == mrCancel) break;
				else if (i == mrNo)
					continue;
			}
			
			strcpy(out, newpath);
			return 1;
		}
		
		Alert("\"%s\" is incorrect name.", name);
	}
	
	return 0;
}

void FileBrowser::SelectAllFiles(void)
{
	int i = numfiles+numdirs;
	while(--i >= 0)
	{
		SCANDIRENT* pFile = &files[i];
		if (pFile->flags & kDirExpFile)
			pFile->flags |= kDirExpSelected;
	}
}

void FileBrowser::UnselectAllFiles(void)
{
	int i = numfiles+numdirs;
	while(--i >= 0)
		files[i].flags &= ~kDirExpSelected;
}

void FileBrowser::ConcatToPath(char* pName)
{
	pathCatSlash(path);
	strcat(path, pName);
}

void FileBrowser::SanitizePath(char* pPath)
{
	strReplace(pPath, '\\', '/');
	pathRemSlash(pPath);
}


/** FileBrowser user functions
********************************************************************************/
char* dirBrowse(char* pAPath, char* pFilter, char* pTitle, int flags)
{
	FileBrowser browser;
	int i = -1, j = 0, c = 0;
	char dirpath[BMAX_PATH] = "\0";
	char filname[BMAX_PATH] = "\0";
	char *pPath;

	if (!isDir(pAPath))
	{
		getFilename(pAPath, filname, 1); getPath(pAPath, dirpath, 0);

		if (!isFile(pAPath))
		{
			filname[0] = '\0';
			if (!isempty(dirpath) && !isDir(dirpath))
			{
				Alert("Could not open \"%s\".", dirpath);
				dirpath[0] = '\0';
			}
		}
	}
	else
	{
		strcpy(dirpath, pAPath);
	}
	
	// maybe expand relative to absolute?
	_fullpath(dirpath, dirpath, BMAX_PATH);
	
	pPath = browser.ShowDialog(dirpath, filname, pFilter, pTitle, flags);
	getcwd(dirpath, sizeof(dirpath));
	
	
	// the caller must prepare enough room
	if (!(flags & kDirExpNoModPtr))
	{
		if (pPath)
		{
			strcpy(pAPath, getRelPath(dirpath, pPath));
		}
		//else if (!(flags & kDirExpNoKeepPath))
		//{
			//strcpy(pAPath, getRelPath(dirpath, browser.path));
		//}
	}
	
	if (!(flags & kDirExpNoFree))
	{
		// make it compact or free it
		
		if (pPath)
		{
			c = dirBrowseCountSelected();
			if (c < browser.numfiles)
			{
				while(dirBrowseEnumSelected(&i, NULL))
				{
					if (i == j)
						continue;
					
					memmove(&browser.files[j++], &browser.files[i], sizeof(SCANDIRENT));
				}
				
				browser.files		= (SCANDIRENT*)realloc(browser.files, c*sizeof(SCANDIRENT));
				browser.numfiles	= c;
				browser.numdirs		= 0;
			}
		}
		else
		{
			if (browser.files)
				free(browser.files), browser.files = NULL;
			
			browser.numfiles	= 0;
			browser.numdirs		= 0;
		}
	}
	
	return pPath ? getRelPath(dirpath, pPath) : NULL;
}

char* browseOpenMany(char* pPath, char* pFilter, char* pTitle, int flags)
{
	return dirBrowse(pPath, pFilter, pTitle, flags|kDirExpMulti);
}

char* browseOpen(char* pPath, char* pFilter, char* pTitle, int flags)
{
	return dirBrowse(pPath, pFilter, pTitle, flags|kDirExpRff);
}

char* browseOpenFS(char* pPath, char* pFilter, char* pTitle, int flags)
{
	return dirBrowse(pPath, pFilter, pTitle, flags);
}

char* browseSave(char* pPath, char* pFilter, char* pTitle, int flags)
{
	return dirBrowse(pPath, pFilter, pTitle, flags|kDirExpSave);
}


char dirBrowseEnumItems(int* idx, char* out, int which)
{
	int nTotal = FileBrowser::numfiles+FileBrowser::numdirs;
	SCANDIRENT* files = FileBrowser::files;
	SCANDIRENT* pFile;
	int nID;

	if (files)
	{
		nID = (*idx < 0) ? 0 : *idx + 1;

		while(nID < nTotal)
		{
			pFile = &files[nID];
			if (pFile->flags & which)
			{
				if (out)
					strcpy(out, pFile->full);
				
				*idx = nID;
				return 1;
			}
			
			nID++;
		}
	}

	return 0;
}

int dirBrowseCountItems(int which)
{
	int nID = -1, cnt = 0;
	while(dirBrowseEnumItems(&nID, NULL, which)) cnt++;
	return cnt;
}

char dirBrowseEnumSelected(int* idx, char* out)
{
	return dirBrowseEnumItems(idx, out, kDirExpSelected);
}

int dirBrowseCountSelected()
{
	return dirBrowseCountItems(kDirExpSelected);
}



static int qsSortDirMoveUp(SCANDIRENT* ref1, SCANDIRENT* ref2)
{
	char dir1 = ((ref1->flags & kDirExpDir) > 0);
	char dir2 = ((ref2->flags & kDirExpDir) > 0);

	if ((dir1 && dir2) || (!dir1 && !dir2)) return 0;
	if ((!dir1 && !dir2) || (!dir1 && dir2))
		return 1;
	
	return -1;
}

static int qsSortByType(SCANDIRENT* ref1, SCANDIRENT* ref2)
{
	return  stricmp(ref1->type, ref2->type);
}

static int qsSortByName(SCANDIRENT* ref1, SCANDIRENT* ref2)
{
	return  stricmp(ref1->name, ref2->name);
}

static void helperAllocThumb(int nTile, int sizeX, int sizeY, int bg)
{
	if (waloff[nTile])
	{
		if (tilesizx[nTile] != sizeX || tilesizy[nTile] != sizeY)
		{
			tileFreeTile(nTile);
			tileAllocTile(nTile, sizeX, sizeY);
		}
	}
	else
	{
		tileAllocTile(nTile, sizeX, sizeY);
	}
	
	if (waloff[nTile] && bg >= 0)
		memset((void*)waloff[nTile], clr2std(bg), tilesizx[nTile]*tilesizy[nTile]);
}

static char getThumbnail_PFB(char* filepath, int nTile, int twh, int thg, int bg)
{
	IniFile* pIni = new IniFile(filepath);
	char* pText = NULL, *key, *val, nRetn = 0;
	int nPrevNode = -1, nLen = 0;
	int t;
	
	// make sure it's an XMAPEDIT prefab
	if (!pIni->SectionExists(kPrefabIniSection))
	{
		delete(pIni);
		return 0;
	}
	
	// try to get encoded image from the file
	while(pIni->GetNextString(&key, &val, &nPrevNode, "Thumbnail"))
	{
		if (key && !val)
		{
			t = strlen(key);
			if ((pText = (char*)realloc(pText, nLen + t)) != NULL)
			{
				memcpy(&pText[nLen], key, t);
				nLen += t;
				continue;
			}
		}

		if (nLen)
		{
			nLen = 0;
			free(pText);
		}

		break;
	}

	if (nLen)
	{
		// decode and copy image here
		////////////////////////////////////

		BIN2TXT t2b;
		t2b.txt.ptr = pText; t2b.txt.len = nLen;
		if (t2b.Decode() && t2b.inf.ptr && t2b.inf.len >= 2)
		{
			int32_t twh = t2b.inf.ptr[0];
			int32_t thg = t2b.inf.ptr[1];
			
			if (twh*thg == t2b.bin.len)
			{
				helperAllocThumb(nTile, twh, thg, bg);
				memcpy((void*)waloff[nTile], t2b.bin.ptr, t2b.bin.len);
				nRetn = 1;
			}
		}

		if (t2b.bin.ptr) free(t2b.bin.ptr);
		if (t2b.inf.ptr) free(t2b.inf.ptr);
		free(pText);
	}
	
	if (nRetn == 0)
	{
		// generate a new thumbnail
		////////////////////////////////////

		char tColor = bg;
		int s, e, wh, hg, zhg, bvisib;
		int cama = 0, camx = 0, camy = 0, camz;
		int nSect, nBlank, nAng;
		sectortype* pSect;

		// insert a large enough sector
		if ((nSect = makeSquareSector(0, 0, MET2PIX(512))) < 0)
			return 0;

		pSect = &sector[nSect];
		pSect->ceilingz	= -0x20000;
		pSect->floorz	=  0x20000;

		getSectorWalls(nSect, &s ,&e); camz = pSect->floorz;
		if (pfbInsert(pIni, OBJ_WALL, s, nSect, camx, camy, pSect->floorz, 0) > 0)
		{
			// use blank tile for walls, floors
			// and ceilings so drawrooms
			// will simple ignore it

			nBlank = ClipLow(tileGetBlank(), 0);
			pSect->floorpicnum		= nBlank;
			pSect->ceilingpicnum	= nBlank;
			pSect->floorshade		= 0;
			pSect->ceilingshade		= 0;
			pSect->ceilingheinum	= 0;
			pSect->floorheinum 		= 0;
			pSect->floorstat 		= 0;
			pSect->ceilingstat 		= 0;

			t = s;
			while(t <= e)
				wall[t++].picnum = nBlank;

			// box size info
			pfbGetSize(pIni, &wh, &hg, &zhg);

			if (!wh && !hg)		hg = zhg>>4;
			if (wh < zhg>>4)	swapValues(&wh, &hg);


			if (tColor == kColorMagenta)
				tColor = 22;

			helperAllocThumb(nTile, twh, thg, tColor);

			// setup camera
			camz = pSect->floorz - ClipLow(zhg>>1, 8192);
			cama = (((GetWallAngle(s) + kAng90) & kAngMask) + kAng180) & kAngMask;
			nnExtOffsetPos(0, -(((wh > hg) ? wh : hg)+ClipLow(zhg>>4, 768)), 0, cama, &camx, &camy, NULL); // moves backward
			RotatePoint(&camx, &camy, kAng45, 0, 0);
			cama += kAng45;

			bvisib = visibility, visibility = 0;
			setviewtotile(nTile, thg, twh);
			h = 0;

			drawrooms(camx, camy, camz, cama, 100, nSect);
			viewProcessSprites(camx, camy, camz, cama);
			drawmasks();

			setviewback();
			artedRotateTile(nTile);
			artedFlipTileY(nTile);
			visibility = bvisib;

			if (bg == kColorMagenta)
			{
				// have to replace colors after drawing
				// because magenta may become "redish" under
				// translucent sprites
				
				replaceByte((BYTE*)waloff[nTile], twh*thg, clr2std(tColor), clr2std(kColorMagenta));
			}

			//if (pfbAttachThumbnail(pIni, nTile)) // finally, attach it!
				//pIni->Save();

			hgltReset(kHgltPoint);
			nRetn = 1;
		}

		deletesector(nSect);
	}
	
	delete(pIni);
	return nRetn;
}

static char getThumbnail_MAP(char* filepath, int nTile, int wh, int hg, int bg)
{
	int nRev, nSkyBits, nSkySize, nKey, hFile;
	int nSects = 0, nWalls = 0, nSprs = 0;
	int i, ver = 0, x1, y1, x2, y2;
	int ax = 0, ay = 0;
	char ver7 = 0, c;

	walltype* pWalls = NULL, *pWall; sectortype sect; SCREEN2D screen;
	BLMMAGIC magic; BLMHEADER_MAIN header; BLMHEADER_EXTRA extra;

	if ((hFile = open(filepath, O_BINARY|O_RDONLY, S_IWRITE|S_IREAD)) < 0) return 0;
	if (read(hFile, &magic, sizeof(magic)) != sizeof(magic))
	{
		close(hFile);
		return 0;
	}

	// check if it's a BLOOD map
	if (memcmp(magic.sign, kBloodMapSig, strlen(kBloodMapSig)) == 0)
	{
		// Unfortunately, BLOOD map format is so very fucked up!
		//////////////////////////////////////

		ver7 = ((magic.version & 0xFF00) == (0x0700 & 0xFF00));
		if (!ver7 && (magic.version & 0xFF00) != (0x0603 & 0xFF00))
		{
			close(hFile);
			return 0;
		}

		if (read(hFile, &header, sizeof(header)) != sizeof(header))
		{
			close(hFile);
			return 0;
		}

		if (ver7)
			dbCrypt((char*)&header, sizeof(header), kMattID2);

		nSkyBits	= B_LITTLE16(header.skyBits);
		nRev		= B_LITTLE32(header.revision);
		nSects		= B_LITTLE16(header.numsectors);
		nWalls		= B_LITTLE16(header.numwalls);
		nSprs		= B_LITTLE16(header.numsprites);

		if ((pWalls = (walltype*)malloc(sizeof(walltype)*nWalls)) == NULL)
		{
			close(hFile);
			return 0;
		}

		nSkySize = ClipHigh(1 << nSkyBits, MAXPSKYTILES)*sizeof(int16_t);

		if (ver7)
		{
			// have to read extra header...
			read(hFile, &extra, sizeof(extra));
			dbCrypt((char*)&extra, sizeof(extra), nWalls);

			extra.xsecSiz = B_LITTLE32(extra.xsecSiz);
			extra.xsprSiz = B_LITTLE32(extra.xsprSiz);
			extra.xwalSiz = B_LITTLE32(extra.xwalSiz);
		}
		else
		{
			extra.xsecSiz = kXSectorDiskSize;
			extra.xsprSiz = kXSpriteDiskSize;
			extra.xwalSiz = kXWallDiskSize;
		}

		// at least can skip sky tiles!
		lseek(hFile, nSkySize, SEEK_CUR);

		// have to read all sectors...
		nKey = nRev*sizeof(sectortype);
		for (i = 0; i < nSects; i++)
		{
			read(hFile, &sect, sizeof(sectortype));
			if (ver7)
				dbCrypt((char*)&sect, sizeof(sectortype), nKey);

			// skip XSECTOR info
			if (B_LITTLE16(sect.extra) >= 0)
				lseek(hFile, extra.xsecSiz, SEEK_CUR);
		}

		// have to read all walls...
		nKey = (nRev*sizeof(sectortype)) | kMattID2;
		for (i = 0; i < nWalls; i++)
		{
			pWall = &pWalls[i];
			read(hFile, pWall, sizeof(walltype));
			if (ver7)
				dbCrypt((char*)pWall, sizeof(walltype), nKey);

			pWall->x 			= B_LITTLE32(pWall->x);
			pWall->y 			= B_LITTLE32(pWall->y);
			pWall->extra 		= B_LITTLE16(pWall->extra);
			pWall->point2 		= B_LITTLE16(pWall->point2);
			pWall->nextwall 	= B_LITTLE16(pWall->nextwall);

			if (!rngok(pWall->point2, 0, nWalls))
			{
				free(pWalls); close(hFile);
				return 0;
			}

			// skip XWALL info
			if (B_LITTLE16(pWall->extra) >= 0)
				lseek(hFile, extra.xwalSiz, SEEK_CUR);
		}

	}
	// maybe it's a BUILD map?
	else
	{
		lseek(hFile, 0, SEEK_SET);
		read(hFile, &ver, 4);
		lseek(hFile, 4+4+4+2+2, SEEK_CUR); // we only need a version

		switch(ver)
		{
			case 7L:
			case 8L:
			case 9L:
				read(hFile, &nSects, 2);
				lseek(hFile, sizeof(sectortype)*nSects, SEEK_CUR);
				read(hFile, &nWalls, 2);
				if ((pWalls = (walltype*)malloc(sizeof(walltype)*nWalls)) != NULL)
					read(hFile, pWalls, sizeof(walltype)*nWalls); // that's it!
				break;
			case 6L:
				read(hFile, &nSects, 2);
				lseek(hFile, sizeof(SECTOR6)*nSects, SEEK_CUR);
				read(hFile, &nWalls, 2);
				if ((pWalls = (walltype*)malloc(sizeof(walltype)*nWalls)) != NULL)
				{
					for (i = 0; i < nWalls; i++)
					{
						WALL6 wall;
						pWall = &pWalls[i];
						read(hFile, &wall, sizeof(wall));


						pWall->x 			= wall.x;
						pWall->y 			= wall.y;
						pWall->point2 		= wall.point2;
						pWall->nextwall 	= wall.nextwall;
					}
				}
				break;
			case 4L:
				read(hFile, &nSects, 2);
				read(hFile, &nWalls, 2);
				lseek(hFile, (sizeof(SECTOR4)*nSects)+2, SEEK_CUR);
				if ((pWalls = (walltype*)malloc(sizeof(walltype)*nWalls)) != NULL)
				{
					for (i = 0; i < nWalls; i++)
					{
						WALL4 wall;
						pWall = &pWalls[i];
						read(hFile, &wall, sizeof(wall));


						pWall->x 			= wall.x;
						pWall->y 			= wall.y;
						pWall->point2 		= wall.point2;
						pWall->nextwall 	= wall.nextwall1;
					}
				}
				break;
			default:
				close(hFile);
				return 0;
		}
	}

	if (nWalls <= 0)
	{
		if (pWalls)
			free(pWalls);

		close(hFile);
		return 0;
	}

	// get average point of a whole board
	for (i = 0; i < nWalls; i++)
	{
		ax += pWalls[i].x;
		ay += pWalls[i].y;
	}

	screen.SetView(0, 0, wh, hg);
	screen.data.camx = ax / nWalls;
	screen.data.camy = ay / nWalls;
	screen.data.zoom = 32; 	// don't know how to scale zoom to board size, so use this

	gfxBackupClip();
	gfxSetClip(0, 0, wh, hg);
	helperAllocThumb(nTile, wh, hg, bg);
	setviewtotile(nTile, hg, wh);

	// use 2d mode screen interface to draw lines
	for (i = 0; i < nWalls; i++)
	{
		pWall = &pWalls[i];
		if (pWall->nextwall <= i)
		{
			// draw only one side of the wall
			x1 = screen.cscalex(pWall->x);					y1 = screen.cscaley(pWall->y);
			x2 = screen.cscalex(pWalls[pWall->point2].x);	y2 = screen.cscaley(pWalls[pWall->point2].y);
			screen.DrawLine(x1, y1, x2, y2, clr2std((pWall->nextwall >= 0) ? kColorRed : 16));
		}
	}

	setviewback();
	gfxRestoreClip();

	artedRotateTile(nTile);
	artedFlipTileY(nTile);

	free(pWalls);
	close(hFile);
	return 2;
}

static char getThumbnail_ART(char* filepath, int nTile, int wh, int hg, int bg)
{
	int hFile, t, s, e, six, siy, pnm, dat, bcnt, mapofs, blkofs;
	XRTBLK_HEAD head; XRTBLKb_DATA customPal; PALETTE pal;
	char buff[BMAX_PATH], nRetn = 0;
	int16_t twh, thg;

	if ((hFile = open(filepath, O_BINARY|O_RDONLY, S_IREAD|S_IWRITE)) >= 0)
	{
		if (readArtHead(hFile, &s, &e, &six, &siy, &pnm, &dat) >= 0)
		{
			if (!xartOffsets(hFile, &bcnt, &mapofs))
			{
				getPath(filepath, buff); strcat(buff, "palette.dat");
				if (!fileExists(buff) || palLoad(buff, pal) < 0)
				{
					close(hFile);
					return 0;
				}
			}
			else
			{
				while(bcnt--)													// read all the block offsets from the block map
				{
					mapofs+=read(hFile, &blkofs, sizeof(int));					// read block offset value
					lseek(hFile, blkofs-sizeof(int), SEEK_CUR);					// go to the actual block
					read(hFile, &head, sizeof(head));							// read block header
					if (head.blktype == kArtXBlockGlobalPalette)
					{
						if (head.datasiz != sizeof(customPal)) break;			// check for consistency
						read(hFile, pal, sizeof(pal));							// using custom palette now
						break;
					}
					
					lseek(hFile, mapofs, SEEK_SET);								// back to the block map
				}
				
				if (bcnt <= 0)
				{
					close(hFile);
					return 0;
				}
			}

			while( 1 )
			{
				if (lseek(hFile, six, SEEK_SET) != six) break;
				if (read(hFile, &twh, sizeof(twh)) != sizeof(twh)) break;
				six+=sizeof(twh);

				if (lseek(hFile, siy, SEEK_SET) != siy) break;
				if (read(hFile, &thg, sizeof(thg)) != sizeof(thg)) break;
				siy+=sizeof(thg);
				t = twh*thg;

				if (t > 0)
				{
					helperAllocThumb(nTile, twh, thg, -1);
					if (lseek(hFile, dat, SEEK_SET) != dat) break;
					if (read(hFile, (void*)waloff[nTile], t) == t)
					{
						palFixTransparentColor(pal);
						pal[clr2std(kColorMagenta)] = gamepal[clr2std(bg)];
						remapColors(waloff[nTile], twh*thg, pal);
						nRetn = 2;
					}

					break;
				}
			}
		}

		close(hFile);
	}

	return nRetn;

}

static char getThumbnail_SEQ(char* filepath, int nTile, int wh, int hg, int bg)
{
	Seq* pSeq = NULL; SEQFRAME* pFrame; BYTE* pTile;
	char nRetn = 0;
	int nTile2;
	int i;

	if (fileLoadHelper(filepath, (BYTE**)&pSeq) >= sizeof(Seq))
	{
		if (memcmp(pSeq->signature, kSEQSig, strlen(kSEQSig)) == 0 && (pSeq->version & 0xff00) == 0x0300)
		{
			for (i = pSeq->nFrames>>2; i < pSeq->nFrames; i++)
			{
				nTile2 = seqGetTile(&pSeq->frames[i]);
				if (nTile2 != nTile && (pTile = tileLoadTile(nTile2)) != NULL)
				{
					wh = tilesizx[nTile2]; hg = tilesizy[nTile2];
					helperAllocThumb(nTile, wh, hg, bg);
					memcpy((void*)waloff[nTile], pTile, wh*hg);
					nRetn = 1;
					break;
				}
			}
		}
	}

	if (pSeq)
		free(pSeq);

	return nRetn;
}

static char getThumbnail_QAV(char* filepath, int nTile, int wh, int hg, int bg)
{
	QAV* pQav = NULL; FRAMEINFO* pFrame; TILE_FRAME* pTFrame;
	char nRetn = 0;
	int i, j;

	if (fileLoadHelper(filepath, (BYTE**)&pQav) >= sizeof(pQav))
	{
		if (memcmp(pQav->sign, kQavSig, strlen(kQavSig)) == 0 && (pQav->version & 0xff00) == kQavVersion)
		{
			for (i = pQav->nFrames>>2; i < pQav->nFrames; i++)
			{
				pFrame = &pQav->frames[i];
				j = LENGTH(pFrame->tiles);
				while(--j >= 0)
				{
					pTFrame = &pFrame->tiles[j];
					if (pTFrame->picnum == nTile)
						break;
				}

				if (j > 0)
					continue;

				pQav->Preload();
				helperAllocThumb(nTile, wh, hg, bg);
				setviewtotile(nTile, hg, wh);

				DrawFrame(pQav->x, pQav->y, pTFrame, 0x02, 0, 0);

				setviewback();
				artedRotateTile(nTile);
				artedFlipTileY(nTile);
				nRetn = 1;
				break;
			}
		}
	}

	if (pQav)
		free(pQav);

	return nRetn;
}

static char getThumbnail_PALS(char* filepath, int nTile, int wh, int hg, int bg)
{
	PALETTE pal;
	const int cols = 16;
	if (palLoad(filepath, pal) >= 0)
	{
		int r = cols, c = cols, csz = wh / cols;
		int x = 0, y = 0, i;

		helperAllocThumb(nTile, wh, hg, bg);
		setviewtotile(nTile, hg, wh);

		i = 0;
		while(i < 256)
		{
			RGB rgb = pal[i];
			gfxSetColor(scrFindClosestColor(rgb.r, rgb.g, rgb.b));
			gfxFillBox(x+1, y+1, x+csz-1, y+csz-1);

			i++;
			if (--r) x+=csz;
			else if (--c) y+=csz, x = 0, r = cols;
			else break;
		}

		setviewback();
		artedRotateTile(nTile);
		artedFlipTileY(nTile);

		return 2; // draw transparent color
	}

	return 0;
}

static char getThumbnail_PICS(char* filepath, int nTile, int wh, int hg, int bg)
{
	static int nImageTile = -1;
	int iwh, ihg, sz;

	if (nImageTile >= 0 || (nImageTile = tileGetBlank()) >= 0)
	{
		IMG2TILEFUNC pFunc = imgGetConvFunc(imgGetType(filepath));
		if (!pFunc || imgFile2TileFunc(pFunc, filepath, nImageTile) < 0)
			return 0;

		iwh = tilesizx[nImageTile];
		ihg = tilesizy[nImageTile];
		
		if (iwh <= wh && ihg <= hg)
		{
			// safe to just copy the whole tile
			helperAllocThumb(nTile, iwh, ihg, bg);
			memcpy((void*)waloff[nTile], (void*)waloff[nImageTile], iwh*ihg);
		}
		else
		{
			// to reduce cache size we clamp
			// image size to the max thumb
			// size
			
			sz = (wh < hg) ? wh : hg;						// define the max size
			tileDrawGetSize(nImageTile, sz, &iwh, &ihg);	// get new width and height
			sz = (iwh < ihg) ? ihg : iwh;					// set largest possible size
			
			helperAllocThumb(nTile, iwh, ihg, bg);
			
			setviewtotile(nTile, ihg, iwh);
			tileDrawTile((int)0, (int)0, (short)nImageTile, (int)sz, (short)0, (char)0x02, (signed char)0);
			setviewback();
			
			artedRotateTile(nTile);
			artedFlipTileY(nTile);
		}
		
		tileFreeTile(nImageTile);
		return 1;
	}

	return 0;
}