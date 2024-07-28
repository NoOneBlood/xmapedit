/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Additional changes by NoOne.
// Functions to handle both 3d and 2d editing modes.
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


#ifndef __XMPSTUB_H
#define __XMPSTUB_H
#include "edit2d.h"
#include "edit3d.h"

#define kPlayerRadius		32
#define kTabWall  kMaxWalls
#define kTabXWall kMaxXWalls
#define kTabSpr  kMaxSprites
#define kTabXSpr kMaxXSprites
#define kTabSect  kMaxSectors
#define kTabXSect kMaxXSectors
#define kHClock 32

#define ED2D (qsetmode != 200)
#define ED3D (qsetmode == 200)
#define ED23 gSplitMode.mode
#define kSplitModeWndPad 2

extern OBJECT_LIST gModernTypes;

enum {
kDetailSpriteNoVoxel1 		= 0x0008,
};

#define kBufferSize 		256
extern char buffer[kBufferSize];
extern char buffer2[kBufferSize];
extern char buffer3[kBufferSize];
extern char gMapInfoStr[kBufferSize];

struct SPECIAL_DATA_NAMES
{
	int var1;
	char *dataOvr[4];
};

extern SPECIAL_DATA_NAMES pCtrlDataNames[32];

extern char h;
extern BYTE key, ctrl, alt, shift;
extern short gHighSpr, gHovSpr;
extern short gHovWall, gHovStat;
extern short temptype, tempidx;
extern char tempvisibility;
extern short tempang, tempslope;

extern int cpyspritecnt;
extern spritetype cpysprite[kMaxSprites + 1];
extern XSPRITE cpyxsprite[kMaxXSprites + 1];

extern int cpysectorcnt;
extern sectortype cpysector[kMaxSectors + 1];
extern XSECTOR cpyxsector[kMaxXSectors + 1];

extern int cpywallcnt;
extern walltype cpywall[kMaxWalls + 1];
extern XWALL cpyxwall[kMaxXWalls + 1];

extern char *gSpriteNames[1024];
extern char *gSpriteCaptions[1024];
extern char *gSpriteDataNames[1024][4];

extern char *gWallNames[1024];
extern char *gWallDataNames[1024][1];

extern char *gSectorNames[1024];
extern char *gSectorDataNames[1024][1];

extern char *gBoolNames[2];
extern char *gBusyNames[4];
extern char *gWaveNames[12];
extern char *gCommandNames[256];
extern char *gRespawnNames[4];
extern char *gKeyItemNames[8];
extern char *gSearchStatNames[8];
extern char *gZModeNames[4];
extern char* gDepthNames[8];
extern char* gDamageNames[7];
extern NAMED_TYPE gDifficNames[6];
extern NAMED_TYPE gGameNames[5];
extern NAMED_TYPE gGameObjectGroupNames[10];

enum {
kCaptionStyleNone			= 0,
kCaptionStyleMapedit		= 1,
kCaptionStyleBuild			= 2,
kCaptionStyleMax			   ,
};

extern char *gCaptionStyleNames[kCaptionStyleMax];

enum {
kToolDialog			= -1,
kToolMapEdit		=  0,
kToolSeqEdit		=  1,
kToolArtEdit		=  2,
kToolQavEdit		=  3,
kToolMax				,
};

enum {
kMap				= 0,
kSeq				= 1,
kArt				= 2,
kQav				= 3,
kExtMax				   ,
};

extern char* gExtNames[kExtMax];
extern NAMED_TYPE gToolNames[kToolMax];


enum {
kIniDefault 					= 0,
kIniEditDialogHints				= 1,
};

// modal results for 3d esc menu
enum {
mrSave 							= 100,
mrSaveAs 						= 101,
mrLoad 							= 102,
mrReload						= 103,
mrNew 							= 104,
mrQuit 							= 105,
mrMenu 							= 106,
mrAsave 						= 107,
mrAsaveMax 						= 113,
mrLoadAsave 					= mrAsaveMax,
mrTest							,
mrOptions						,
mrAbout							,
mrBoardOptions					,
mrToolPreviewMode				,
mrToolDoorWizard				,
mrToolSpriteText				,
mrToolExpSeq					,
mrToolCleanChannel				,
mrToolImportWizard				,
mrToolArtGrabber				,
mrToolSeqEdit					,
mrToolArtEdit					,
mrToolQavEdit					,
};

enum {
kOGrpNone 						= 0x0000,
kOGrpModern     				= 0x0001,
kOGrpWeapon						= 0x0002,
kOGrpAmmo						= 0x0004,
kOGrpAmmoMix 					= 0x0008,
kOGrpItem						= 0x0010,
kOGrpDude						= 0x0020,
kOGrpHazard						= 0x0040,
kOGrpMisc						= 0x0080,
kOGrpMarker						= 0x0100,
};

/// !!!
/* enum {
VCLS							= 0, // show NON plasma types (the difference in enemies)
VPLS							= 1, // show plasma types as well
VHID							= 2, // show hidden vanilla types
MDRN							= 4, // show modern types
}; */

enum {
VC 								= 0,
VA 								= 1, // additional types that compatible with vanilla and could be useful for mappers
MO 								= 2,
};

enum {
kPrefixModernOnly 				= '*',
kPrefixSpecial					= '~',
};


int GetXSector( int nSector );
int GetXWall( int nWall );
int GetXSprite( int nSprite );


BOOL processKeysShared();
void processDrawRooms(int camx, int camy, int camz, int cama, int camh, int nSect);
inline void processDrawRooms() { processDrawRooms(posx,posy,posz,ang,horiz,cursectnum); }
char* onOff(int var);
char* isNot(int var);
char* yesNo(int var);
int xmpMenuCreate(char* name);
int xmpMenuProcess();
void xmpQuit(int code = 0);
void xmpOptions(void);
void xmpSetEditMode(char nMode);
int xsysConnect(int nTypeA, int nIdxA, int nTypeB, int nIdxB);
int xsysConnect2(int nTypeA, int nIdxA, int nTypeB, int nIdxB);
void processMove();
void setStartPos(int x, int y, int z, int ang, char forceEditorPos);
int boardLoad(char *filename);
void boardPreloadTiles();
int boardSave(char* filename, BOOL autosave);
int boardSaveAuto();
void boardReset(int hgltreset = 0);
void boardStartNew();

const char *ExtGetSectorCaption(short sectnum, char captStyle);
const char *ExtGetWallCaption(short wallnum, char captStyle);
const char *ExtGetSpriteCaption(short spritenum, char captStyle);
#endif