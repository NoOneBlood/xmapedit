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
#include "iob.h"

enum {
kFlgMapTypeBlood	= 0x0001,
kFlgCompareTiles	= 0x0002,
kFlgImportAnim		= 0x0004,
kFlgImportView		= 0x0008,
kFlgKeepOffset		= 0x0020,
kFlgOverwrite		= 0x0040,
kFlgNoTileMap		= 0x0080,
kFlgReplace			= 0x0100,
};

enum {
kGrabFloor			= 0x01,
kGrabCeil			= 0x02,
kGrabWall			= 0x04,
kGrabOWall			= 0x08,
kGrabSprite			= 0x20,
kGrabAll			= kGrabFloor|kGrabCeil|kGrabWall|kGrabOWall|kGrabSprite,
};


extern char* toolMenu[3];

struct ARTFILE;
struct POINT {
	
	int x, y;
	
};

#include "xmparted.h"


#pragma pack(push, 1)
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

struct IMPORT_WIZARD_MAP_ARG
{
	char* filepath;
	IOBuffer* pIo;
	unsigned int blood		: 1;
	unsigned int allowSel	: 1;
	int version;
};
#pragma pack(pop)

class IMPORT_WIZARD
{	
	#define kWizardVerMajor 1
	#define kWizardVerMinor 0
	
	private:
	unsigned int nStartTile	: 17;
	unsigned int nArtFiles	: 16;
	unsigned int selpal		: 1;
	unsigned int selmap		: 1;
	ARTFILE* pArtFiles;
	PALETTE pal;
	BYTE* pMapData;
	BOOL bloodMap;
	int mapVersion;
	
	public:
	IMPORT_WIZARD()  { Init(); };
	~IMPORT_WIZARD() { Free(); };
	void Init();
	void Free();
	// ---------------------------------------------------------------------
	int  ShowDialog(IMPORT_WIZARD_MAP_ARG* pMapArg = NULL);
	void DlgDisableButton(TextButton* pButton);
	void DlgEnableButton(TextButton* pButton, char fontColor = kColorBlack);
	void DlgDisableCheckbox(Checkbox* pCheck, int checked = -1);
	void DlgEnableCheckbox(Checkbox* pCheck, int checked = -1, char fontColor = kColorBlack);
	// ---------------------------------------------------------------------
	void MapEraseSprites();
	void MapErasePalookups(char nPalookup = 0);
	void MapEraseInfo();
	void MapClipPicnums();
	void MapAdjustShade(int nPalookus);
	int  MapCheckFile(IOBuffer* pIo, int* pSuppVerDB, int DBLen, BOOL* bloodMap);
	// ---------------------------------------------------------------------
	void ArtSetStartTile();
	
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
BOOL toolLoadAs(char* path, char* ext, char* title = "Load file");
BOOL toolSaveAs(char* path, char* ext);

void cpyRestoreObject(int type, int idx);
void cpySaveObject(int type, int idx);

int toolExploderSeq();
int toolGibTool(int nGib, int type = -1);
int toolXWalls2XSprites();

int updViewAngle(spritetype* pSprite);
void chgSpriteZ(spritetype* pSprite, int zVal);
int toolMapArtGrabber(ARTFILE* files, int nFiles, PALETTE pal, int nStartTile, int skyTiles, int flags, int grabFrom = kGrabAll);
int toolOpenWith(char* filename, char flags = 0x0);
//int toolGetPluDlg(int nTile, int nDefaultRetn = -1);
#endif