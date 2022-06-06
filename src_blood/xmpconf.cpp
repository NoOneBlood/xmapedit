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

#include <stdio.h>
#include <string.h>

#include "xmpstub.h"
#include "common_game.h"
#include "editor.h"
#include "aadjust.h"
#include "misc.h"
#include "gui.h"
#include "xmpsnd.h"
#include "xmpmisc.h"
#include "xmpconf.h"


IniFile* MapEditINI = NULL;
int gMaxTiles;

AUTOADJUST gAutoAdjust;
AUTOSAVE gAutosave;
AUTOGRID gAutoGrid;
BEEP gBeep;
COMMENT_SYS_PREFS gCmtPrefs;
COMPATIBILITY gCompat;
LIGHT_BOMB gLightBomb;
IMPORT_WIZARD_PREFS gImportPrefs;
MAPEDIT_HUD_SETTINGS gHudPrefs;
MISC_PREFS gMisc;
MOUSE gMouse;
OBJECT_LOCK gObjectLock;
PATHS gPaths;
ROTATION gRotateOpts;
SCREEN gScreen;
SOUND gSound;
TILE_VIEWER gTileView;
TIMERS gTimers;

void AUTOSAVE::Init(IniFile* pIni, char* section)
{
	max = ClipRange(pIni->GetKeyInt(section, "MaxSaveCopies", 1), 1, 999);
	interval = 120 * pIni->GetKeyInt(section, "SaveInterval", 5 * 60);
	sprintf(basename, "%0.7s", pIni->GetKeyString(section, "FileName", "ASAVE"));
}

void AUTOADJUST::Init(IniFile* pIni, char* section)
{
	setType = TRUE;
	if ((enabled = pIni->GetKeyBool(section, "Enabled", TRUE)) == 1)
	{
		setPic 			= pIni->GetKeyBool(section, "SetPicnum",	TRUE);
		setSize			= pIni->GetKeyBool(section, "SetSize",		TRUE);
		setPlu			= pIni->GetKeyBool(section, "SetPalette",	TRUE);
		setHitscan		= pIni->GetKeyBool(section, "SetHitscan",	TRUE);	
		setStatnum 		= pIni->GetKeyBool(section, "SetStatnum",	TRUE);

		// modify autoData array
		short pic, xr, yr, plu;
		for (int i = 0; i < autoDataLength; i++)
		{
			if (autoData[i].seq < 0) continue;
			else if (getSeqPrefs(autoData[i].seq, &pic, &xr, &yr, &plu))
			{
				autoData[i].picnum  = pic;
				autoData[i].xrepeat = xr;
				autoData[i].yrepeat = yr;
				autoData[i].plu     = plu;
				autoData[i].xsprite = TRUE; // if it have seq, then it must be xsprite
			}
		}
	}
}

void AUTOGRID::Init(IniFile* pIni, char* section)
{
	enabled = pIni->GetKeyBool(section, "Enabled", FALSE);
	for (int i = 0, value = kStartGridValue; i < kMaxGrids; i++)
	{
		char tmp[16];
		sprintf(tmp, "ZoomForGrid%d", i + 1);
		zoom[i] = pIni->GetKeyInt(section, tmp, value*=kGridMul);
	}
}

void AUTOGRID::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "Enabled", enabled);
}

void BEEP::Init() {
    
	int i, len; char* p;
	freq = MixRate, volume = 16; //ClipLow(FXVolume >> 8, 4);
    
	// BeepOk
	if (!data[kBeepOk])
	{
		length[kBeepOk] = len = 2 * freq / 120;
		data[kBeepOk]   = (char*)malloc(len);
		p = data[kBeepOk];
		
		for (i = 0; i < len; i++)
		{
			int amm = mulscale30(127, Sin(i * (6000 * 2048 / freq)));
			int rest = len - i - 1;
			if (rest < 32)
				amm = mulscale5(amm, rest);
			p[i] = 128 + amm;
		}
    }
	
	// BeepFail
    if (!data[kBeepFail])
	{
		length[kBeepFail] = len = 8 * freq / 120;
		data[kBeepFail]   = (char*)malloc(len);
		p = data[kBeepFail];
		
		for (i = 0; i < len >> 1; i++)
		{
			if (((i * 2000) / freq) % 2 == 0)
				p[i] = 255;
			else
				p[i] = 0;
		}
		
		for (i = 0; i < len >> 1; i++)
		{
			if (((i * 1600) / freq) % 2 == 0)
				p[i+len/2] = 255;
			else
				p[i+len/2] = 0;
		}
	}
}

void BEEP::Play(int type)
{
	if (type < kBeepOk || type >= kBeepMax || !data[type])
		return;
	
	Stop();	
	hBeep = FX_PlayRaw(data[type], length[type], freq, 0, volume, volume, volume, 128, (unsigned int)&hBeep);
}

void BEEP::Stop()
{
	if (hBeep > 0)
		FX_StopSound(hBeep);
}

void COMMENT_SYS_PREFS::Init(IniFile* pIni, char* section)
{
	enabled		= pIni->GetKeyBool(section, "Enabled", TRUE);
	compareCRC	= pIni->GetKeyBool(section, "CompareCRC", TRUE);
}

void COMMENT_SYS_PREFS::Save(IniFile* pIni, char* section)
{
	//pIni->PutKeyInt(section, "Enabled", enabled);
}

void COMPATIBILITY::Init(IniFile* pIni, char* section)
{
	modernMap	= pIni->GetKeyBool(section, "SaveAsModernMap", FALSE);
	indicate 	= pIni->GetKeyBool(section, "IndicateCompat", TRUE);
	maxTiles 	= ClipHigh(pIni->GetKeyInt(section, "MaxTiles", 3), 3);
	switch (maxTiles) {
		case 1:
			gMaxTiles = 4096;
			break;
		case 2:
			gMaxTiles = 6144;
			break;
		default:
			gMaxTiles = kMaxTiles - 128;
			break;
	}
}

void IMPORT_WIZARD_PREFS::Init(IniFile* pIni, char* section)
{
	mapErsSpr 		= pIni->GetKeyBool(section, "MapEraseSprites", 		0);
	mapErsPal 		= pIni->GetKeyBool(section, "MapErasePalettes", 	1);
	mapErsInfo	 	= pIni->GetKeyBool(section, "MapEraseObjectInfo", 	1);
	mapImpotArt 	= pIni->GetKeyBool(section, "MapImportArt", 		1);
	artKeepOffset 	= pIni->GetKeyBool(section, "ArtKeepTilenums", 		1);
	artImportAnim 	= pIni->GetKeyBool(section, "ArtImportAnim", 		1);
	artImportView 	= pIni->GetKeyBool(section, "ArtImportView", 		1);
	artTileMap 		= pIni->GetKeyBool(section, "ArtTileMap", 			1);
	artChgTilenums 	= pIni->GetKeyBool(section, "ArtChangeTilenums", 	1);
	artNoDuplicates = pIni->GetKeyBool(section, "ArtNoDuplicates", 		1);
}

void IMPORT_WIZARD_PREFS::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "MapEraseSprites", 	mapErsSpr);
	pIni->PutKeyInt(section, "MapErasePalettes", 	mapErsPal);
	pIni->PutKeyInt(section, "MapEraseObjectInfo", 	mapErsInfo);
	pIni->PutKeyInt(section, "MapImportArt", 		mapImpotArt);
	pIni->PutKeyInt(section, "ArtKeepTilenums", 	artKeepOffset);
	pIni->PutKeyInt(section, "ArtImportAnim", 		artImportAnim);
	pIni->PutKeyInt(section, "ArtImportView", 		artImportView);
	pIni->PutKeyInt(section, "ArtTileMap", 			artTileMap);
	pIni->PutKeyInt(section, "ArtChangeTilenums", 	artChgTilenums);
	pIni->PutKeyInt(section, "ArtNoDuplicates", 	artNoDuplicates);
}

void LIGHT_BOMB::Init(IniFile* pIni, char* section)
{
	intensity 			= pIni->GetKeyInt(section, "Intensity", 16);
	attenuation 		= pIni->GetKeyInt(section, "Attenuation", 0x1000);
	reflections 		= pIni->GetKeyInt(section, "Reflections", 2);
	maxBright 			= (char)pIni->GetKeyInt(section, "MaxBright", -4);
	rampDist 			= pIni->GetKeyInt(section, "RampDist", 0x10000);
}

void MAPEDIT_HUD_SETTINGS::Init(IniFile* pIni, char* section)
{
	layout3D				= ClipHigh(pIni->GetKeyInt(section, "Layout3D", kHudLayoutFull), kHudLayoutMax);
	dynamicLayout3D			= 0;
	
	layout2D				= ClipHigh(pIni->GetKeyInt(section, "Layout2D", kHudLayoutDynamic), kHudLayoutMax);
	dynamicLayout2D			= (layout2D == kHudLayoutDynamic);
	if (dynamicLayout2D)
		layout2D = kHudLayoutFull;
	
	tileShowShade			= pIni->GetKeyBool(section, "TileShowShade", FALSE);
	tileScaleSize			= ClipHigh(pIni->GetKeyInt(section, "TileSizeOutside", 100), 1023);
}

void MAPEDIT_HUD_SETTINGS::Save(IniFile* pIni, char* section)
{
	if (dynamicLayout3D) layout3D = kHudLayoutDynamic;
	if (dynamicLayout2D) layout2D = kHudLayoutDynamic;
	pIni->PutKeyInt(section, "Layout3D", layout3D);
	pIni->PutKeyInt(section, "Layout2D", layout2D);
}

void MISC_PREFS::Init(IniFile* pIni, char* section)
{
	this->pan				= 1;
	this->palette			= 0;
	this->autoSecrets 		= pIni->GetKeyBool(section,	"AutoCountSecrets", 0);
	this->beep				= pIni->GetKeyBool(section, "Beep", TRUE);
	this->diffSky			= pIni->GetKeyBool(section, "MultipleSky", FALSE);
	this->useCaptions		= pIni->GetKeyBool(section, "UseCaptions", TRUE);
	this->showTypes			= pIni->GetKeyInt(section, "ShowAdvancedTypes", 1);
	this->hgltTreshold 		= pIni->GetKeyInt(section, "HighlightThreshold", 40);
	this->zeroTile2pal 		= ClipHigh(pIni->GetKeyInt(section, "FirstTile2Color", 0), 254);
	this->forceSetup		= pIni->GetKeyBool(section, "ForceSetup", 1);
	this->externalModels	= ClipHigh(pIni->GetKeyInt(section, "ShowExternalModels", 2), 2);
	this->zlockAvail		= !pIni->GetKeyBool(section, "SkipZStepMode", TRUE);
	
	sprintf(this->tilesBaseName, (!fileExists("TILES000.ART")) ? "SHARE" : "TILES");
	if ((this->ambShowRadius = pIni->GetKeyInt(section, "ShowAmbientRadius", 4)) == 4)
	{
		this->ambShowRadiusHglt	= 1;
		this->ambShowRadius		= 3;
	}
	
	this->autoLoadMap	 	= pIni->GetKeyBool(section, "AutoLoadMap", FALSE); 
	this->editMode			= pIni->GetKeyBool(section, "EditMode", FALSE);
	
	usevoxels 				= (!externalModels || externalModels == 2) ? 0 : 1;
	grid 					= (short)ClipHigh(pIni->GetKeyInt(section, "GridSize", 4), kMaxGrids - 1);
	kensplayerheight 		= pIni->GetKeyInt(section, "EyeHeight", 58) << 8;
	zmode 					= ClipHigh(pIni->GetKeyInt(section, "ZMode", 0), 3);
	zoom 					= ClipRange(pIni->GetKeyInt(section, "Zoom", 768), 24, 8192);
	showtags				= pIni->GetKeyInt(section, "CaptionStyle", kCaptionStyleMapedit);
	editorgridextent		= ClipRange(196608, kDefaultBoardSize >> 1, kDefaultBoardSize << 2);
	
}



void MISC_PREFS::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "ZMode", zmode);
	pIni->PutKeyInt(section, "EditMode", (qsetmode == 200));
	pIni->PutKeyInt(section, "GridSize", grid);
	pIni->PutKeyInt(section, "Zoom", zoom);
	pIni->PutKeyInt(section, "Beep", beep);
	pIni->PutKeyInt(section, "ShowAmbientRadius", ambShowRadius + ambShowRadiusHglt);
	pIni->PutKeyInt(section, "ForceSetup", forceSetup);
	pIni->PutKeyInt(section, "CaptionStyle", showtags);
	pIni->PutKeyInt(section, "EyeHeight", kensplayerheight >> 8);

}

void MOUSE_LOOK::Init(IniFile* pIni, char* section)
{
	strafe		= pIni->GetKeyBool(section, "Turn2Strafe", 0);
	mode		= ClipHigh(pIni->GetKeyInt(section, "Mode", 3), 3);
	dir    		= ClipHigh(pIni->GetKeyInt(section, "Direction", 3),  3);
	invert 		= ClipHigh(pIni->GetKeyInt(section, "Invert", 0), 3);
	maxSlope	= ClipRange(pIni->GetKeyInt(section, "MaxSlope", 240), 32, 400);
	maxSlopeF	= ClipHigh(maxSlope, (widescreen) ? 100 : 200);
}

void MOUSE::Init(IniFile* pIni, char* section)
{
	speedX = speedXInit	= ClipRange(pIni->GetKeyInt(section, "SpeedX", 50), 10, 512);
	speedY = speedYInit = ClipRange(pIni->GetKeyInt(section, "SpeedY", 100), 10, 512);	
	controls 			= ClipRange(pIni->GetKeyInt(section, "Controls", 1), 0, 3);
	fixedGrid			= ClipRange(pIni->GetKeyInt(section, "FixedGridSize", 5), 0, kMaxGrids - 1);
	
	press				= 0;
	hold				= 0;
	release				= 0;
	wheel				= 0;
	acceleration		= 1.12;
	rangeX1				= 0;
	rangeX2				= 320;
	rangeY1				= 0;
	rangeY2				= 200;
	X					= (rangeX2 - rangeX1) >> 1;
	Y					= (rangeY2 - rangeY1) >> 1;
	dX					= 0;
	dY					= 0;
	dX2					= 0;
	dY2					= 0;
	dfX					= 0;
	dfY					= 0;
	
	look.Init(pIni, "MouseLook");
}

void MOUSE::SetRange(int x1, int y1, int x2, int y2, bool scaleSpeed)
{
	dfX |= (X << 16);
	dfY |= (Y << 16);
	
	speedScale = scaleSpeed;
	
	if (speedScale)
	{
		dfX = scale(dfX, x2 - x1, rangeX2 - rangeX1);
		dfY = scale(dfY, y2 - y1, rangeY2 - rangeY1);
	}
	else
	{
		dfX = scale(dfX, x2 - x1, xdim);
		dfY = scale(dfY, y2 - y1, ydim);
	}
	
	X = dfX >> 16;
	Y = dfY >> 16;

	dfX &= 0xFFFF;
	dfY &= 0xFFFF;
	
	rangeX1 = x1;
	rangeY1 = y1;
	rangeX2 = x2;
	rangeY2 = y2;
}

void MOUSE::PushRange() {
	
	if (stackCount >= LENGTH(rangeStack))
		ThrowError("Mouse range stack overflow!");
	
	MOUSE_RANGE* pRange = &rangeStack[stackCount];
	pRange->X1 = rangeX1;	pRange->X2 = rangeX2;
	pRange->Y1 = rangeY1;	pRange->Y2 = rangeY2;
	stackCount++;
}

void MOUSE::PopRange() {
	
	if (stackCount <= 0)
		return;

	stackCount--;
	MOUSE_RANGE* pRange = &rangeStack[stackCount];
	SetRange(pRange->X1, pRange->Y1, pRange->X2, pRange->Y2);
}

void MOUSE::Read(int nTicks)
{
	ResetSpeedSmooth();
	
	int mx = 0, my = 0, dummy;
	getmousevalues(&mx, &my, &dummy);
	ReadButtons();
	
	if (buttons & 16)		wheel = -1;
	else if (buttons & 32)	wheel = 1;
	else					wheel = 0;
	

/* 	if ( nTicks > 0 )
	{
		mx = qsgn(mx) * nTicks * pow((double)klabs(mx) / nTicks, acceleration);
		my = qsgn(my) * nTicks * pow((double)klabs(my) / nTicks, acceleration);
	} */
	
	mx *= speedX;
	my *= speedY;
	
	if (speedScale)
	{
		dfX += mx * (rangeX2 - rangeX1);
		dfY += my * (rangeY2 - rangeY1);
	}
	else
	{
		dfX += mx * xdim;
		dfY += my * ydim;
	}
	
	dX2 = dfX >> 16;
	dY2 = dfY >> 16;

	dfX &= 0xFFFF;
	dfY &= 0xFFFF;

	dX = X;
	dY = Y;

	X = ClipRange(X + dX2, rangeX1, rangeX2 - 1);
	Y = ClipRange(Y + dY2, rangeY1, rangeY2 - 1);

	dX = X - dX;
	dY = Y - dY;
	
}

int MOUSE::ReadButtons()
{
	ResetSpeedSmooth();
	readmousebstatus(&buttons);
	
	release = 0;
	if ((hold & 1) && !(buttons & 1)) release |= 1;
	if ((hold & 2) && !(buttons & 2)) release |= 2;
	if ((hold & 4) && !(buttons & 4)) release |= 4;
	
	press	= (~hold & buttons);
	hold	= buttons;
	
	return buttons;
}

int MOUSE::ReadWheel()
{
	ResetSpeedSmooth();
	int tmp; readmousebstatus(&tmp);
	if (tmp & 16)		wheel = -1;
	else if (tmp & 32)	wheel = 1;
	else				wheel = 0;
	
	return wheel;
}

void MOUSE::ResetSpeedNow()
{
	speedX		= speedXInit;
	speedY 		= speedYInit;
	speedReset	= FALSE;
}

void MOUSE::ResetSpeedSmooth()
{
	// smooth mouse speed reset
	if (speedReset && totalclock >= gTimers.mouseSpeedReset + 4)
	{
		if (speedX < gMouse.speedXInit) speedX = ClipHigh(speedX + 1, speedXInit);
		else if (speedX > speedXInit) speedX = ClipLow(speedX - 1, speedXInit);
		
		if (speedY < speedYInit) speedY = ClipHigh(speedY + 1, speedYInit);
		else if (speedY > gMouse.speedYInit) speedY = ClipLow(speedY - 1, speedYInit);
		
		if (speedX == speedXInit && speedY == speedYInit) speedReset = 0;
		else gTimers.mouseSpeedReset = totalclock;
	}
}

void MOUSE::ClampSpeed(int speedX, int speedY)
{
	ResetSpeedNow();
	this->speedX = this->speedXInit = speedX;
	this->speedY = this->speedYInit = speedY;
}


void MOUSE::Save(IniFile* pIni, char*)
{
	pIni->PutKeyInt("MouseLook", "Mode", look.mode);
}

void MOUSE::ChangeCursor(short id)
{
	cursor.type = id;
	if (id > 0)
	{
		cursor.width	= pBitmaps[id]->width;
		cursor.height	= pBitmaps[id]->height;
		cursor.pad		= 0;
	}
	else
	{
		cursor.width	= mulscale16(10, 0x10000);
		cursor.height	= mulscale16(10, 0x10000);
		cursor.pad		= mulscale16(4,  0x10000);
	}
	
	cursor.xoffs = 0;
	cursor.yoffs = 0;
}

void MOUSE::Draw() {
	
	int i;
	int nCur = cursor.type;
	int xoffs = cursor.xoffs;
	int yoffs = cursor.yoffs;
	int wh = cursor.width;
	int hg = cursor.height;
	int pd = cursor.pad;

	switch (nCur) {
		default:
			if (nCur > 0 && pBitmaps && pBitmaps[nCur])
			{
				gfxDrawBitmap(nCur, X - 1, Y - 1);
				break;
			}
			fallthrough__;
			// no break
		case 0:
			if (qsetmode == 200)
			{
				gfxSetColor(fade());
				gfxHLine(Y, X-wh, X-pd);
				gfxHLine(Y, X+pd, X+wh);
				gfxVLine(X, Y-hg, Y-pd);
				gfxVLine(X, Y+pd, Y+hg);
				gfxPixel(X, Y);
			}
			else
			{
				pd>>=1;
				gfxSetColor(gStdColor[gridlock ? kColorLightRed : kColorWhite]);
				for (i = 0; i < 2; i++)
				{
					gfxHLine(Y+i, X-wh, X-pd);
					gfxHLine(Y+i, X+pd, X+wh);
					gfxVLine(X+i, Y-hg, Y-pd);
					gfxVLine(X+i, Y+pd, Y+hg);
				}

			}
			break;
	}
}

void OBJECT_LOCK::Init()
{
	type = -1;
	idx = -1;
}

void PATHS::InitBase()
{
	sprintf(prefabs, 		kPrefabsDir);
	sprintf(palImport, 		kPalImportDir);
	sprintf(palPaint, 		kPalPaintDir);
	memset(images, 	   0, 	sizeof(images));
	memset(episodeIni, 0, 	sizeof(episodeIni));
	memset(maps, 	   0, 	sizeof(maps));
}

void PATHS::InitResourceRFF(IniFile* pIni, char* section)
{
	sprintf(bloodRFF, pIni->GetKeyString(section, "Blood", "BLOOD.RFF"));
	sprintf(soundRFF, pIni->GetKeyString(section, "Sound", "SOUNDS.RFF"));
}

void PATHS::InitResourceART(IniFile* pIni, char* section)
{
	sprintf(surfDAT,  pIni->GetKeyString(section, "Surfaces", "SURFACE.DAT"));
	sprintf(voxelDAT, pIni->GetKeyString(section, "Voxels", "VOXEL.DAT"));
	sprintf(voxelEXT, pIni->GetKeyString(section, "ExternalModels", kExternalVoxelsDB));
	sprintf(shadeDAT, pIni->GetKeyString(section, "Shades", "SHADE.DAT"));
}

void PATHS::InitMisc(IniFile* pIni, char* section)
{
	RESHANDLE hFile; char* pth;
	pth = pIni->GetKeyString(section, "Map", "");
	if (fileExists(pth, &hFile))
		sprintf(maps, pth);
}

void PATHS::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyString(section, "Map", maps);
	//pIni->PutKeyString(section, "Palettes", palImport);
	//pIni->PutKeyString(section, "Palookups", palPaint);
	//pIni->PutKeyString(section, "Images", images);
	//pIni->PutKeyString(section, "Prefabs", prefabs);
}

void ROTATION::Init(IniFile* pIni, char* section)
{
	chgMarkerAng	= ClipRange(pIni->GetKeyInt(section, "ChangeMarkersAngle", 1), 0, 3);
}

void SCREEN::Init(IniFile* pIni, char* section)
{
	xdim2d = xdimgame			= ClipLow(pIni->GetKeyInt(section, "Width", xdimgame), 640);
	ydim2d = ydimgame			= ClipLow(pIni->GetKeyInt(section, "Height", ydimgame), 480);
	fullscreen					= pIni->GetKeyBool(section, "Fullscreen", 0);
	bpp							= 8;
}

void SOUND::Init(IniFile* pIni, char* section)
{
	FXDevice 					= 0;
	FXVolume					= ClipRange(pIni->GetKeyInt(section, "SoundVolume", 255), 0, 255);
	MusicVolume					= ClipRange(pIni->GetKeyInt(section, "MusicVolume", 255), 0, 255);
	MusicDevice					= 0;
	NumVoices					= 32;
	NumChannels					= 2;
	NumBits						= 16;
	MixRate						= 44100;
	ReverseStereo				= 0;
	
	sndInit();
}

void SCREEN::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "Width", xdimgame);
	pIni->PutKeyInt(section, "Height", ydimgame);
	pIni->PutKeyInt(section, "Fullscreen", fullscreen);
}

void TILE_VIEWER::Init(IniFile* pIni, char* section)
{
	tilesPerRow		= ClipRange(pIni->GetKeyInt(section, "TilesPerRow", 8), 3, 14);
	background 		= ClipHigh(pIni->GetKeyInt(section, "BackgroundColor", 8), 15);
	transBackground	= pIni->GetKeyBool(section, "TranslucentBackground", TRUE);
	showAnimation	= pIni->GetKeyBool(section, "ShowAnimation", TRUE);
	showTransColor	= pIni->GetKeyBool(section, "ShowTransparentColor", FALSE);
	showInfo		= pIni->GetKeyBool(section, "AlwaysShowTileInfo", TRUE);
	stretch			= pIni->GetKeyBool(section, "StretchTiles", FALSE);
	bglayers		= 2;
	
	InitWindow();
}

void TILE_VIEWER::InitWindow()
{
	wx1				= 1;		// border
	wy1				= 14;		// title
	wx2				= xdim-1;	// border
	wy2				= ydim-15;  // status bar + border
}

void TILE_VIEWER::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "TilesPerRow", tilesPerRow);
	pIni->PutKeyInt(section, "ShowAnimation", showAnimation);
	pIni->PutKeyInt(section, "ShowTransparentColor", showTransColor);
	pIni->PutKeyInt(section, "StretchTiles", stretch);
	pIni->PutKeyInt(section, "AlwaysShowTileInfo", showInfo);
	pIni->PutKeyInt(section, "TranslucentBackground", transBackground);
}

void::TIMERS::Init(int clock)
{
	//memset(this, clock, sizeof(this));
}