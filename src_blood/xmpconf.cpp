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

#include "xmpmaped.h"
#include "xmpconf.h"
#include "xmpsnd.h"


IniFile* MapEditINI = NULL;
int gMaxTiles;

AUTOADJUST gAutoAdjust;
AUTOSAVE gAutosave;
AUTOGRID gAutoGrid;
BEEP gBeep;
COMMENT_SYS_PREFS gCmtPrefs;
COMPATIBILITY gCompat;
EXTERNAL_APPS gExtApps;
FILEBROWSER_PREFS gDirBroPrefs;
LIGHT_BOMB gLightBomb;
IMPORT_WIZARD_PREFS gImportPrefs;
MAPEDIT_HUD_SETTINGS gHudPrefs;
MISC_PREFS gMisc;
MOUSE_PREFS gMousePrefs;
MOUSE_LOOK  gMouseLook;
OBJECT_LOCK gObjectLock;
PATHS gPaths;
PLUPICKER gPluPrefs;
ROTATION gRotateOpts;
SCREEN gScreen;
SOUND gSound;
SPLITMODE_DATA gSplitMode;
TILE_VIEWER gTileView;
TIMERS gTimers;

void AUTOSAVE::Init(IniFile* pIni, char* section)
{
    max = ClipRange(pIni->GetKeyInt(section, "MaxSaveCopies", 1), 1, 999);
    interval = 120 * pIni->GetKeyInt(section, "SaveInterval", 5 * 60);
    strcpy(buffer, pIni->GetKeyString(section, "FileName", "ASAVE"));
    getFilename(buffer, basename, 0);
}

void AUTOSAVE::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "MaxSaveCopies", max);
    pIni->PutKeyInt(section, "SaveInterval", interval / 120);
    pIni->PutKeyString(section, "FileName", basename);
}

void AUTOADJUST::Init(IniFile* pIni, char* section)
{
    setType = TRUE;
    if ((enabled = pIni->GetKeyBool(section, "Enabled", TRUE)) == 1)
    {
        setPic              = pIni->GetKeyBool(section, "SetPicnum",            TRUE);
        setSize             = pIni->GetKeyBool(section, "SetSize",              TRUE);
        setPlu              = pIni->GetKeyBool(section, "SetPalette",           TRUE);
        setHitscan          = pIni->GetKeyBool(section, "SetHitscan",           TRUE);
        setStatnum          = pIni->GetKeyBool(section, "SetStatnum",           TRUE);
        setStatnumThings    = pIni->GetKeyBool(section, "SetStatnumForThings",  TRUE);
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
    hBeep = FX_PlayRaw(data[type], length[type], freq, 0, volume, volume, volume, 128, (uintptr_t)&hBeep);
}

void BEEP::Stop()
{
    if (hBeep > 0)
        FX_StopSound(hBeep);
}

void COMMENT_SYS_PREFS::Init(IniFile* pIni, char* section)
{
    enabled     = pIni->GetKeyBool(section, "Enabled", TRUE);
    compareCRC  = pIni->GetKeyBool(section, "CompareCRC", TRUE);
}

void COMMENT_SYS_PREFS::Save(IniFile* pIni, char* section)
{
    //pIni->PutKeyInt(section, "Enabled", enabled);
}

void COMPATIBILITY::Init(IniFile* pIni, char* section)
{
    modernMap   = pIni->GetKeyBool(section, "SaveAsModernMap", FALSE);
    indicate    = pIni->GetKeyBool(section, "IndicateCompat", TRUE);
    maxTiles    = ClipHigh(pIni->GetKeyInt(section, "MaxTiles", 3), 3);
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

void EXTERNAL_APPS::Init(IniFile* pIni, char* section)
{
    char *key, *val, *qs, *qe; int nNode = -1;
    EXTERNAL_APP* pApp;

    memset(&apps, 0, sizeof(apps));
    numapps = 0;

    while(numapps < kMaxExtApps && pIni->GetNextString(&key, &val, &nNode, section))
    {
        pApp = &apps[numapps];
        if (!isempty(val) || isempty(key))
            continue;

        qs = qe = NULL;
        switch(strQuotPtr(key, &qs, &qe))
        {
            case 0:
                strcpy(pApp->path, key);
                break;
            case 2:
                strSubStr(key, qs+1, qe, pApp->path); qe++;
                if (*qe == '\0') qe = NULL;
                break;
            default:
                continue;
        }

        if (qe)
        {
            strcpy(pApp->cmd, qe);
            strTrim(pApp->cmd);
        }

        strTrim(pApp->path);


        numapps++;
    }
}

void FILEBROWSER_PREFS::Init(IniFile* pIni, char* section)
{
    previewArea     = pIni->GetKeyBool(section, "ShowPreview", 1);
    previewStretch  = pIni->GetKeyBool(section, "StretchPreview", 0);
    thumbnails      = pIni->GetKeyBool(section, "ShowThumbnails", 1);
    cols            = ClipHigh(pIni->GetKeyInt(section, "Cols", 0), 16);
    rows            = ClipHigh(pIni->GetKeyInt(section, "Rows", 0), 16);

}

void FILEBROWSER_PREFS::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "StretchPreview", previewStretch);
    pIni->PutKeyInt(section, "ShowThumbnails", thumbnails);
    pIni->PutKeyInt(section, "ShowPreview", previewArea);
    pIni->PutKeyInt(section, "Cols", cols);
    pIni->PutKeyInt(section, "Rows", rows);
}

void IMPORT_WIZARD_PREFS::Init(IniFile* pIni, char* section)
{
    mapErsSpr       = pIni->GetKeyBool(section, "MapEraseSprites",      0);
    mapErsPal       = pIni->GetKeyBool(section, "MapErasePalettes",     1);
    mapErsInfo      = pIni->GetKeyBool(section, "MapEraseObjectInfo",   1);
    mapImpotArt     = pIni->GetKeyBool(section, "MapImportArt",         1);
    artKeepOffset   = pIni->GetKeyBool(section, "ArtKeepTilenums",      1);
    artImportAnim   = pIni->GetKeyBool(section, "ArtImportAnim",        1);
    artImportView   = pIni->GetKeyBool(section, "ArtImportView",        1);
    artTileMap      = pIni->GetKeyBool(section, "ArtTileMap",           1);
    artChgTilenums  = pIni->GetKeyBool(section, "ArtChangeTilenums",    1);
    artNoDuplicates = pIni->GetKeyBool(section, "ArtNoDuplicates",      1);
}

void IMPORT_WIZARD_PREFS::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "MapEraseSprites",     mapErsSpr);
    pIni->PutKeyInt(section, "MapErasePalettes",    mapErsPal);
    pIni->PutKeyInt(section, "MapEraseObjectInfo",  mapErsInfo);
    pIni->PutKeyInt(section, "MapImportArt",        mapImpotArt);
    pIni->PutKeyInt(section, "ArtKeepTilenums",     artKeepOffset);
    pIni->PutKeyInt(section, "ArtImportAnim",       artImportAnim);
    pIni->PutKeyInt(section, "ArtImportView",       artImportView);
    pIni->PutKeyInt(section, "ArtTileMap",          artTileMap);
    pIni->PutKeyInt(section, "ArtChangeTilenums",   artChgTilenums);
    pIni->PutKeyInt(section, "ArtNoDuplicates",     artNoDuplicates);
}

void LIGHT_BOMB::Init(IniFile* pIni, char* section)
{
    intensity           = pIni->GetKeyInt(section, "Intensity", 16);
    attenuation         = pIni->GetKeyInt(section, "Attenuation", 0x1000);
    reflections         = pIni->GetKeyInt(section, "Reflections", 2);
    maxBright           = (signed char)pIni->GetKeyInt(section, "MaxBright", -4);
    rampDist            = pIni->GetKeyInt(section, "RampDist", 0x10000);
}

void MAPEDIT_HUD_SETTINGS::Init(IniFile* pIni, char* section)
{
    layout3D                = ClipHigh(pIni->GetKeyInt(section, "Layout3D", kHudLayoutFull), kHudLayoutMax);
    dynamicLayout3D         = 0;

    layout2D                = ClipHigh(pIni->GetKeyInt(section, "Layout2D", kHudLayoutDynamic), kHudLayoutMax);
    dynamicLayout2D         = (layout2D == kHudLayoutDynamic);
    if (dynamicLayout2D)
        layout2D = kHudLayoutFull;

    layoutSPLIT             = ClipHigh(pIni->GetKeyInt(section, "LayoutSPLIT", kHudLayoutCompact), kHudLayoutMax);
    dynamicLayoutSPLIT      = (layoutSPLIT == kHudLayoutDynamic);
    if (dynamicLayoutSPLIT)
        layoutSPLIT = kHudLayoutFull;

    tileShowShade           = pIni->GetKeyBool(section, "TileShowShade", FALSE);
    tileScaleSize           = ClipHigh(pIni->GetKeyInt(section, "TileSizeOutside", 100), 1023);
    fontPack                = pIni->GetKeyInt(section, "FontPack", 3);
}

void MAPEDIT_HUD_SETTINGS::Save(IniFile* pIni, char* section)
{
    if (dynamicLayoutSPLIT) layoutSPLIT = kHudLayoutDynamic;
    if (dynamicLayout3D)    layout3D = kHudLayoutDynamic;
    if (dynamicLayout2D)    layout2D = kHudLayoutDynamic;


    pIni->PutKeyInt(section, "LayoutSPLIT", layoutSPLIT);
    pIni->PutKeyInt(section, "Layout3D", layout3D);
    pIni->PutKeyInt(section, "Layout2D", layout2D);
    pIni->PutKeyInt(section, "FontPack", fontPack);
}

void MISC_PREFS::Init(IniFile* pIni, char* section)
{
    this->pan               = 1;
    this->palette           = 0;
    this->autoSecrets       = pIni->GetKeyBool(section, "AutoCountSecrets", 0);
    this->beep              = pIni->GetKeyBool(section, "Beep", TRUE);
    this->diffSky           = pIni->GetKeyBool(section, "MultipleSky", FALSE);
    this->useCaptions       = pIni->GetKeyBool(section, "UseCaptions", TRUE);
    this->showTypes         = pIni->GetKeyInt(section, "ShowAdvancedTypes", 1);
    this->hgltTreshold      = pIni->GetKeyInt(section, "HighlightThreshold", 40);
    this->zeroTile2pal      = ClipHigh(pIni->GetKeyInt(section, "FirstTile2Color", 0), 254);
    this->forceSetup        = pIni->GetKeyBool(section, "ForceSetup", 1);
    this->externalModels    = ClipHigh(pIni->GetKeyInt(section, "ShowExternalModels", 2), 2);
    this->zlockAvail        = !pIni->GetKeyBool(section, "SkipZStepMode", TRUE);

    this->autoLoadMap       = pIni->GetKeyBool(section, "AutoLoadMap", FALSE);
    this->editMode          = pIni->GetKeyInt(section, "EditMode", 0);
    this->circlePoints      = 6;
    this->forceEditorPos    = pIni->GetKeyBool(section, "ForceEditorStartPos", FALSE);
    this->undoCamRestore    = pIni->GetKeyBool(section, "MapUndoRestoreCamPos", TRUE);

    gSplitMode.size         = pIni->GetKeyInt(section, "SplitPanelSize", 0);
    gSplitMode.swapSize     = pIni->GetKeyBool(section, "SplitAutoSwapSize", 1);
    gSplitMode.vertical     = pIni->GetKeyBool(section, "SplitVertical", 1);

    SCREEN2D* pScr = &gScreen2D;
    pScr->prefs.useTransluc         = pIni->GetKeyBool(section, "UseTranslucentEffects", TRUE);
    pScr->prefs.showMap             = pIni->GetKeyInt(section, "ShowMap2d", 0);
    pScr->prefs.showTags            = pIni->GetKeyInt(section, "CaptionStyle", kCaptionStyleMapedit);
    pScr->prefs.ambRadius           = pIni->GetKeyInt(section, "ShowAmbientRadius", 4);
    if (pScr->prefs.ambRadius == 4)
    {
        pScr->prefs.ambRadiusHover  = 1;
        pScr->prefs.ambRadius       = 3;
    }

    usevoxels               = (!externalModels || externalModels == 2) ? 0 : 1;
    grid                    = (short)ClipHigh(pIni->GetKeyInt(section, "GridSize", 4), kMaxGrids - 1);
    kensplayerheight        = pIni->GetKeyInt(section, "EyeHeight", 58) << 8;
    zmode                   = ClipHigh(pIni->GetKeyInt(section, "ZMode", 0), 3);
    zoom                    = ClipRange(pIni->GetKeyInt(section, "Zoom", 768), 24, 8192);

    boardWidth              = 196608;
    boardHeight             = boardWidth;

    strcpy(this->tilesBaseName, (!fileExists("TILES000.ART")) ? "SHARE" : "TILES");

}



void MISC_PREFS::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "ZMode", zmode);
    pIni->PutKeyInt(section, "EditMode", (ED23) ? 0x02 : ED3D);
    pIni->PutKeyInt(section, "GridSize", grid);
    pIni->PutKeyInt(section, "Zoom", zoom);
    pIni->PutKeyInt(section, "Beep", beep);
    pIni->PutKeyInt(section, "ForceSetup", forceSetup);
    pIni->PutKeyInt(section, "SplitPanelSize", gSplitMode.size);
    pIni->PutKeyInt(section, "SplitAutoSwapSize", gSplitMode.swapSize);
    pIni->PutKeyInt(section, "SplitVertical", gSplitMode.vertical);
    pIni->PutKeyInt(section, "EyeHeight", kensplayerheight >> 8);
    pIni->PutKeyInt(section, "AutoLoadMap", autoLoadMap);
    pIni->PutKeyInt(section, "AutoCountSecrets", autoSecrets);
    pIni->PutKeyInt(section, "SkipZStepMode", !zlockAvail);
    pIni->PutKeyInt(section, "ForceEditorStartPos", forceEditorPos);

    SCREEN2D* pScr = &gScreen2D;
    pIni->PutKeyInt(section, "ShowAmbientRadius", pScr->prefs.ambRadius + pScr->prefs.ambRadiusHover);
    pIni->PutKeyInt(section, "ShowMap2d", pScr->prefs.showMap);
    pIni->PutKeyInt(section, "CaptionStyle", pScr->prefs.showTags);
    pIni->PutKeyInt(section, "UseTranslucentEffects", pScr->prefs.useTransluc);
}

void MOUSE_LOOK::Init(IniFile* pIni, char* section)
{
    strafe      = pIni->GetKeyBool(section, "Turn2Strafe", 0);
    mode        = ClipHigh(pIni->GetKeyInt(section, "Mode", 3), 3);
    dir         = ClipHigh(pIni->GetKeyInt(section, "Direction", 3),  3);
    invert      = ClipHigh(pIni->GetKeyInt(section, "Invert", 0), 3);
    maxSlope    = ClipRange(pIni->GetKeyInt(section, "MaxSlope", 240), 0, 400);
    maxSlopeF   = ClipHigh(maxSlope, (widescreen) ? 100 : 200);
}

void MOUSE_LOOK::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "Mode", mode);
    pIni->PutKeyInt(section, "MaxSlope", maxSlope);
    pIni->PutKeyInt(section, "Invert", invert);
    pIni->PutKeyInt(section, "Turn2Strafe", strafe);
}


void MOUSE_PREFS::Init(IniFile* pIni, char* section)
{
    speedX              = ClipRange(pIni->GetKeyInt(section, "SpeedX", 50), 10, 2048);
    speedY              = ClipRange(pIni->GetKeyInt(section, "SpeedY", 100), 10, 2048);
    controls            = ClipRange(pIni->GetKeyInt(section, "Controls", 1), 0, 3);
    fixedGrid           = ClipRange(pIni->GetKeyInt(section, "FixedGridSize", 5), 0, kMaxGrids - 1);

}

void MOUSE_PREFS::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "SpeedX", speedX);
    pIni->PutKeyInt(section, "SpeedY", speedY);
}

void OBJECT_LOCK::Init()
{
    type = -1;
    idx = -1;
}

void PATHS::InitBase()
{
    sprintf(prefabs,        kPrefabsDir);
    sprintf(palImport,      kPalImportDir);
    sprintf(palPaint,       kPalPaintDir);
    memset(images,     0,   sizeof(images));
    memset(episodeIni, 0,   sizeof(episodeIni));
    memset(maps,       0,   sizeof(maps));
    memset(modNblood,  0, sizeof(modNblood));
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
    char* pth = pIni->GetKeyString(section, "Map", "");
    RESHANDLE hFile; int l;

    if (fileExists(pth, &hFile))
        sprintf(maps, pth);

    sprintf(modNblood, "./");
    sprintf(buffer, "nblood.cfg");

    if (fileExists(buffer))
    {
        IniFile* pConf = new IniFile(buffer);
        pth = pConf->GetKeyString("Setup", "ModDir", NULL);
        if (pth)
        {
            sprintf(modNblood, "%s", pth);
            removeQuotes(modNblood); l = strlen(modNblood);
            if (l == 1 && modNblood[0] == '/' || modNblood[0] == '\\')
                sprintf(modNblood, "./");
        }

        delete(pConf);
    }
}

void PATHS::Save(IniFile* pIni, char* section)
{
    if (!isFile(maps))
    {
        RESHANDLE hRes;
        if (!fileExists(maps, &hRes))
        {
            pIni->KeyRemove(section, "Map");
            return;
        }

    }

    pIni->PutKeyString(section, "Map", maps);
}

void PLUPICKER::Init(IniFile* pIni, char* section)
{
    classicWindow       = pIni->GetKeyBool(section, "ClassicWindow",        FALSE);
    reflectShade        = pIni->GetKeyBool(section, "ReflectShade",         TRUE);
    showAll             = pIni->GetKeyBool(section, "ShowAllPalookups",     FALSE);
    mostEfficentInTop   = pIni->GetKeyBool(section, "MostEfficicentInTop",  TRUE);

}

void PLUPICKER::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "ClassicWindow",           classicWindow);
    pIni->PutKeyInt(section, "ReflectShade",            reflectShade);
    pIni->PutKeyInt(section, "ShowAllPalookups",        showAll);
    pIni->PutKeyInt(section, "MostEfficicentInTop",     mostEfficentInTop);
}


void ROTATION::Init(IniFile* pIni, char* section)
{
    chgMarkerAng    = ClipRange(pIni->GetKeyInt(section, "ChangeMarkersAngle", 1), 0, 3);
}

void SCREEN::Init(IniFile* pIni, char* section)
{
    msgShowTotal                = 1;
    msgShowCur                  = 0;
    msgFont                     = 0;
    msgTime                     = 160;

    gGamma                      = ClipRange(pIni->GetKeyInt(section, "Gamma", 0), 0, 10);
    xdim2d = xdimgame           = ClipLow(pIni->GetKeyInt(section, "Width", 640), 640);
    ydim2d = ydimgame           = ClipLow(pIni->GetKeyInt(section, "Height", 480), 480);
    fullscreen                  = pIni->GetKeyInt(section, "Fullscreen", 0);
    maxFPS                      = pIni->GetKeyInt(section, "MaxFPS", -2);
    bpp                         = 8;
}

void SCREEN::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "Width", xdimgame);
    pIni->PutKeyInt(section, "Height", ydimgame);
    pIni->PutKeyInt(section, "Fullscreen", fullscreen);
    pIni->PutKeyInt(section, "Gamma", gGamma);
    pIni->PutKeyInt(section, "MaxFPS", maxFPS);
}

void SOUND::Init(IniFile* pIni, char* section)
{
    FXDevice                    = 0;
    FXVolume                    = ClipRange(pIni->GetKeyInt(section, "SoundVolume", 255), 0, 255);
    MusicVolume                 = ClipRange(pIni->GetKeyInt(section, "MusicVolume", 255), 0, 255);
    ambientAlways               = pIni->GetKeyBool(section, "AmbientAlways", 0);
    MusicDevice                 = 0;
    NumVoices                   = 32;
    NumChannels                 = 2;
    NumBits                     = 16;
    MixRate                     = 44100;
    ReverseStereo               = 0;

    sndInit();
}

void SOUND::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "SoundVolume", FXVolume);
    pIni->PutKeyInt(section, "MusicVolume", MusicVolume);
    pIni->PutKeyInt(section, "AmbientAlways", ambientAlways);
}

void TILE_VIEWER::Init(IniFile* pIni, char* section)
{
    tilesPerRow     = ClipRange(pIni->GetKeyInt(section, "TilesPerRow", 8), 3, 14);
    background      = ClipHigh(pIni->GetKeyInt(section, "BackgroundColor", 8), 15);
    transBackground = pIni->GetKeyBool(section, "TranslucentBackground", TRUE);
    showAnimation   = pIni->GetKeyBool(section, "ShowAnimation", TRUE);
    showTransColor  = pIni->GetKeyBool(section, "ShowTransparentColor", FALSE);
    showInfo        = pIni->GetKeyBool(section, "AlwaysShowTileInfo", TRUE);
    stretch         = pIni->GetKeyBool(section, "StretchTiles", FALSE);
    bglayers        = 2;

    InitWindow();
}

void TILE_VIEWER::InitWindow()
{
    wx1             = 1;        // border
    wy1             = 14;       // title
    wx2             = xdim-1;   // border
    wy2             = ydim-15;  // status bar + border
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

int initKeyMapper()
{
    char *pIniKey, *pIniVal, *filename = kKeyMapperFile;
    int nNode = -1, nSrc, nDst, c = 0;
    RESHANDLE hIni; IniFile* pIni;

    if (!fileExists(filename) && (hIni = gGuiRes.Lookup(6, "INI")) != NULL)
    {
        pIni = new IniFile((BYTE*)gGuiRes.Load(hIni), gGuiRes.Size(hIni));
        pIni->Save(filename);
        delete(pIni);
    }

    pIni = new IniFile(filename);
    while(pIni->GetNextString(&pIniKey, &pIniVal, &nNode, "KeyRemap"))
    {
        if ((nSrc = strtol(pIniKey, NULL, 16)) == 0 || (nDst = strtol(pIniVal, NULL, 16)) == 0)
            continue;

        if (rngok(nSrc, 0, 256) && rngok(nDst, 0, 256))
        {
            wm_remapkey(nSrc, nDst);
            c++;
        }
    }

    delete(pIni);
    return c;
}