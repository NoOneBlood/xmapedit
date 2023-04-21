/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Initialization & configuration of xmapedit.
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

#ifndef __XMPCONF
#define __XMPCONF

#include "inifile.h"
#include "common_game.h"

#define kConfigReqVersion	2

#define kDefaultMapName		"newboard.map"
#define kXmpDataDir			"xmapedit"
#define kPrefabsDir			"xmapedit/prefabs"
#define kCoreIniName 		"xmapedit/xmapedit.ini"
#define kCoreRffName 		"xmapedit/xmapedit.rff"
#define kExternalVoxelsDB 	"xmapedit/models.ini"
#define kPalDBIni			"xmapedit/palettes/i2tpaldb.ini"
#define kPalImportDir		"xmapedit/palettes/import"
#define kPalPaintDir		"xmapedit/palettes/paint"
#define kLogFile			"xmapedit/xmapedit.log"
#define kStdPalDB			"xmapedit/stdclr.ini"
#define kTempFileBase		"xmapedit/fil"
#define kDefaultBoardSize	196608

#define kMaxGrids 			11
#define kGridMul 			2
#define kStartGridValue 	100

enum {
kBeepOk			= 0,
kBeepFail		= 1,
kBeepMax		   ,
};

extern int gMaxTiles;
extern IniFile* MapEditINI;

class AUTOSAVE {
	public:
	unsigned int max				: 10;
	unsigned int current			: 10;
	unsigned int interval			: 32;
	char basename[8];
	void Init(IniFile* pIni, char* section);
};

class AUTOADJUST {
	public:
	unsigned int enabled			: 1;
	unsigned int setStatnum			: 1;
	unsigned int setType			: 1;
	unsigned int setPic				: 1;
	unsigned int setSize			: 1;
	unsigned int setPlu				: 1;
	unsigned int setHitscan			: 1;
	unsigned int correctedSprites	: 16;
	unsigned int setStatnumThings	: 1;
	void Init(IniFile* pIni, char* section);
};

class AUTOGRID {
	public:
	unsigned int enabled			: 1;
	unsigned int zoom[kMaxGrids];
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};


class BEEP {
	
	protected:
	char* data[kBeepMax];
	unsigned int length[kBeepMax];
	unsigned int freq, volume;
	unsigned int hBeep;
	
	public:
	void Init();
	void Play(int type);
	void Stop();
	
};

class COMPATIBILITY {
	public:
	unsigned int indicate			: 1;
	unsigned int modernMap			: 1;
	unsigned int maxTiles			: 2;
	void Init(IniFile* pIni, char* section);
};

class COMMENT_SYS_PREFS {
	public:
	unsigned int enabled			: 1;
	unsigned int compareCRC			: 1;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class MISC_PREFS {
	public:
	unsigned int pan				: 1;
	unsigned int hgltTreshold		: 6;
	unsigned int zeroTile2pal		: 8;
	unsigned int ambShowRadius		: 4;
	unsigned int ambShowRadiusHglt	: 1;
	unsigned int autoSecrets		: 1;
	unsigned int showTypes			: 2;
	unsigned int diffSky			: 1;
	unsigned int useCaptions		: 1;
	unsigned int autoLoadMap		: 1;
	unsigned int palette			: 8;
	unsigned int externalModels		: 2;
	unsigned int beep 				: 1;
	unsigned int forceSetup			: 1;
	unsigned int zlockAvail			: 1;
	unsigned int editMode			: 1;
	unsigned int useTranslucency	: 1;
	char tilesBaseName[5];
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class LIGHT_BOMB {
	public:
	int intensity;
	int attenuation;
	int reflections;
	int rampDist;
	char maxBright;
	void Init(IniFile* pIni, char* section);
};

class IMPORT_WIZARD_PREFS {
	public:
	unsigned int mapErsSpr			: 1;
	unsigned int mapErsPal			: 1;
	unsigned int mapErsInfo			: 1;
	unsigned int mapImpotArt		: 1;
	unsigned int artKeepOffset		: 1;
	unsigned int artImportAnim		: 1;
	unsigned int artImportView		: 1;
	unsigned int artTileMap			: 1;
	unsigned int artChgTilenums		: 1;
	unsigned int artNoDuplicates	: 1;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class MAPEDIT_HUD_SETTINGS {
	public:
	unsigned int layout2D			: 4;
	unsigned int dynamicLayout2D	: 1;
	unsigned int layout3D			: 4;
	unsigned int dynamicLayout3D	: 1;
	unsigned int tileShowShade		: 1;
	unsigned int tileScaleSize		: 10;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class MOUSE_LOOK {
	
	public:
	unsigned int mode				: 2;
	unsigned int dir				: 2;
	unsigned int invert				: 2;
	unsigned int maxSlope			: 9;
	unsigned int maxSlopeF			: 9;
	unsigned int strafe				: 1;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class MOUSE_PREFS {
	
	public:
	unsigned int controls			: 2;
	unsigned int fixedGrid			: 5;
	unsigned int speedX				: 10;
	unsigned int speedY				: 10;
	void Init(IniFile* pIni, char* section);
};


class OBJECT_LOCK {
	public:
	unsigned int mode				: 3; // 1 3d, 2 2d, 3 both
	short type; 				   		 // searchstat
	short idx; 					   		 // -1 lock on specific object type
	int time; 					    	 // -1 lock forever
	void Init();
};

class PATHS {
	public:
	char maps[_MAX_PATH];
	char prefabs[_MAX_PATH];
	char seqs[_MAX_PATH];
	char qavs[_MAX_PATH];
	char episodeIni[_MAX_PATH];
	char images[_MAX_PATH];
	char palImport[_MAX_PATH];
	char palPaint[_MAX_PATH];
	char bloodRFF[_MAX_PATH];
	char soundRFF[_MAX_PATH];
	char surfDAT[_MAX_PATH];
	char voxelDAT[_MAX_PATH];
	char shadeDAT[_MAX_PATH];
	char voxelEXT[_MAX_PATH];
	char modNblood[_MAX_PATH];
	void InitBase();
	void InitResourceRFF(IniFile* pIni, char* section);
	void InitResourceART(IniFile* pIni, char* section);
	void InitMisc(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class PLUPICKER
{
	public:
	unsigned int classicWindow		: 1;
	unsigned int reflectShade		: 1;
	unsigned int mostEfficentInTop	: 1;
	unsigned int showAll			: 1;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class ROTATION {
	public:
	int chgMarkerAng		: 3; // allow: 1 path markers 2 sector markers
	void Init(IniFile* pIni, char* section);
};

struct SCRMESSAGE
{
	uint32_t time;
	char text[256];
};

class SCREEN {
	public:
	SCRMESSAGE msg[16];
	unsigned int msgShowTotal	: 8;
	unsigned int msgShowCur		: 8;
	unsigned int msgFont		: 8;
	unsigned int msgTime		: 32;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

class SOUND {
	public:
	void Init(IniFile* pIni, char* section);
};

class TILE_VIEWER {
	public:
	unsigned int tilesPerRow		: 4;
	unsigned int background			: 8;
	unsigned int bglayers			: 2;
	unsigned int wx1				: 16;
	unsigned int wx2				: 16;
	unsigned int wy1				: 16;
	unsigned int wy2				: 16;
	BYTE* scrSave;
	BOOL showAnimation;
	BOOL showTransColor;
	BOOL stretch;
	BOOL showInfo;
	BYTE transBackground;
	char title[256];
	char palookup;
	void Init(IniFile* pIni, char* section);
	void InitWindow();
	void Save(IniFile* pIni, char* section);
};

class TIMERS
{
	public:
	uint32_t diskPic;
	uint32_t mouseWheelTimer1;
	uint32_t autosave;
	uint32_t compatCheck;
	uint32_t hudObjectInfo;
	uint32_t sectRL;
	uint32_t pan;
	uint32_t tileScroll;
	
	void Init(int clock);
};

extern AUTOADJUST gAutoAdjust;
extern AUTOSAVE gAutosave;
extern AUTOGRID gAutoGrid;
extern BEEP gBeep;
extern COMMENT_SYS_PREFS gCmtPrefs;
extern COMPATIBILITY gCompat;
extern LIGHT_BOMB gLightBomb;
extern IMPORT_WIZARD_PREFS gImportPrefs;
extern MAPEDIT_HUD_SETTINGS gHudPrefs;
extern MISC_PREFS gMisc;
extern MOUSE_PREFS gMousePrefs;
extern MOUSE_LOOK  gMouseLook;
extern OBJECT_LOCK gObjectLock;
extern PATHS gPaths;
extern PLUPICKER gPluPrefs;
extern ROTATION gRotateOpts;
extern SCREEN gScreen;
extern SOUND gSound;
extern TILE_VIEWER gTileView;
extern TIMERS gTimers;
#endif