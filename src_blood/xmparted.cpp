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

#include "common_game.h"
#include "fire.h"
#include "tile.h"
#include "img2tile.h"
#include "xmpmaped.h"
#include "xmparted.h"
#include "xmptools.h"
#include "xmpexplo.h"
#include "xmpevox.h"


int nArtFiles = 0;
BOOL gDirtTiles[kMaxTiles];
STATUS_BIT1 gHgltTiles[kMaxTiles];
ARTFILE gArtFiles[kMaxArtFiles];
ARTEDIT gArtEd;

char* gArtEdModeNames[3] = {

    "Tile viewer",
    "Batch tile edit",
    "Single tile edit",

};

NAMED_TYPE viewMenu[] = {
    { 0, "Change &view type" },
    { 1, "Change internal &RFF Voxel ID" },
};

NAMED_TYPE artedSaveMenu[] = {
    { 0, "&Save all changes" },
    { 1, "&Revert all changes" },
};

NAMED_TYPE exportMenu[] = {
    { 1, "32bit &TGA image(s)" },
    { 0, "Build Engine &ART file" },
#ifdef I2TDEBUG
    { 2, "8bit &QBM images"},
#endif
//  { 3, "8bit &PCX images (editart)"},
};

NAMED_TYPE importMenu[] = {
    { 0, "Import ima&ges" },
    { 1, "Import Build Engine &ART files" },
    { 2, "Import Chasm: The Rift &FLOOR files"},
};

NAMED_TYPE paintMenu[] = {
    { 1, "&Edit current tile colors..." },
    { 0, "&Paint tiles with palette" },
};

NAMED_TYPE gArtSigns[] = {

    {kArtSignBAFED,     "BAFED"},       // Build Art Files Editor
    {kArtSignBUILDART,  "BUILDART"},    // Ion Fury uses it
};

NAMED_TYPE replaceColorSaveMenu[] = {
    { 0, "Save as &palookup..." },
    { 1, "Modify image &colors directly" },
};

CHECKBOX_LIST revertMenu[] = {
    { TRUE, "&Image and offsets." },
    { TRUE, "&View and animation properties." },
    { TRUE, "&Surface type, shade and RFF voxel id." },
};

NAMED_TYPE gCopyErrors[] = {

    {-1, "Copying beyond max tiles"},
    {-2, "One of tiles overlaps copying"},
    {-999, "Unknown copy error"},
};

NAMED_TYPE gArtFileErrors[] = {

    {-1,    "File is corrupted or not an ART file"},
    {-2,    "Unsupported ART file format"},
    {-999,  "Unknown ART file error"},

};

BOOL artedProcessEditKeys(BYTE key, BYTE ctrl, BYTE alt, BYTE shift) {

    int nTile = gArtEd.nTile;
    int nVTile = gArtEd.nVTile;
    int i, j;

    if (gArtEd.mode == kArtEdModeBatch)
    {
        switch (key) {
            case KEY_TILDE:
                if (!canEdit()) break;
                else if (!shift) hgltTileToggle(nTile);
                else hgltTileSelectRange(nTile);
                scrSetMessage("%d tiles selected.", hgltTileCount());
                return TRUE;
        }
    }
    else if (gArtEd.mode == kArtEdModeTile2D)
    {
        switch (key) {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_RIGHT:
            case KEY_LEFT:
            case KEY_PAD5:
                if (!canEdit()) break;
                switch(key) {
                    case KEY_UP:
                        if (!shift) panm[nVTile].ycenter = ClipHigh(panm[nVTile].ycenter + 1, 127);
                        else panm[nVTile].ycenter = ClipHigh(tilesizy[nVTile] - tilesizy[nVTile] / 2, 127);
                        break;
                    case KEY_DOWN:
                        if (!shift) panm[nVTile].ycenter = ClipLow(panm[nVTile].ycenter - 1, -128);
                        else panm[nVTile].ycenter = ClipLow(-tilesizy[nVTile] / 2, -128);
                        break;
                    case KEY_RIGHT:
                        if (!shift) panm[nVTile].xcenter = ClipLow(panm[nVTile].xcenter - 1, -128);
                        else panm[nVTile].xcenter = ClipHigh(-tilesizx[nVTile] / 2, 127);
                        break;
                    case KEY_LEFT:
                        if (!shift) panm[nVTile].xcenter = ClipHigh(panm[nVTile].xcenter + 1, 127);
                        else panm[nVTile].xcenter = ClipLow(tilesizx[nVTile] - tilesizx[nVTile] / 2, -128);
                        break;
                    case KEY_PAD5:
                        panm[nVTile].xcenter = 0;
                        panm[nVTile].ycenter = 0;
                        break;
                }
                artedArtDirty(nVTile, kDirtyPicanm);
                scrSetMessage("Tile #%d x-offset=%d, y-offset=%d.", nVTile, panm[nVTile].xcenter, panm[nVTile].ycenter);
                return TRUE;
            case KEY_COMMA:
            case KEY_PERIOD:
            case KEY_SLASH:
                if (!canEdit()) break;
                switch (viewType[nTile]) {
                    case kSprViewFull5:
                    case kSprViewFull8:
                        if (key == KEY_SLASH) gTool.nOctant = 0;
                        else if (key == KEY_COMMA) gTool.nOctant = DecRotate(gTool.nOctant, 8);
                        else gTool.nOctant = IncRotate(gTool.nOctant, 8);
                        scrSetMessage("View angle is %ddeg.", gTool.nOctant * 45);
                        break;
                }
                return TRUE;
            case KEY_PADSTAR:
            case KEY_PADSLASH:
                if (!ctrl)
                {
                    i = (key == KEY_PADSTAR) ? 0x1000 : -0x1000;
                    gTool.tileZoom = ClipRange(gTool.tileZoom + i, 0x1000, 0x10000 << 4);
                }
                else
                {
                    gTool.tileZoom = toolGetDefTileZoom();
                }
                scrSetMessage("Zoom: %d%%.", (gTool.tileZoom * 100) / toolGetDefTileZoom());
                return TRUE;
        }
    }

    switch (key) {
        case KEY_F2:
            if (!gArtEd.asksave) scrSetMessage("There is no changes.");
            else if ((i = (showButtons(artedSaveMenu, LENGTH(artedSaveMenu), "Save ART changes") - mrUser)) >= 0)
            {
                switch ( i ) {
                    case 0:
                        artedSaveChanges();
                        scrSetMessage("Changes were saved.");
                        break;
                    case 1:
                        if (!Confirm("Are you sure you want to revert all changes?")) break;
                        artedRevertChanges();
                        scrSetMessage("Changes were reverted.");
                        break;
                }
            }
            return TRUE;
        case KEY_F3:
            if ((i = artedDlgSaveSelection(nVTile)) < 0) return TRUE;
            else if (i == 0) scrSetMessage("You must highlight some tiles.");
            else scrSetMessage("%d tiles saved.", i);
            return TRUE;
        case KEY_F4:
            if (!canEdit()) break;
            while ((i = (showButtons(importMenu, LENGTH(importMenu), "Select action") - mrUser)) >= 0)
            {
                switch ( i ) {
                    case 0:
                        if ((j = artedDlgImportImage(nTile)) == -1) break;
                        scrSetMessage("%d images has been imported.", j);
                        break;
                    case 1:
                        artedDlgImportArt(nTile);
                        break;
                    case 2:
                        artedDlgImportFLOORS(nTile);
                        break;
                }

                break;
            }
            return TRUE;
        case KEY_F5:
            if (!canEdit()) break;
            else if (artedDlgViewTypeSet(nTile) == -1) return TRUE;
            else if (!isExternalModel(nTile)) return TRUE;
            else Alert("Note: Selected view settings may be overriden by external voxel!");
            return TRUE;
        case KEY_F6:
            if (!canEdit()) break;
            else if ((i = showButtons(surfNames, LENGTH(surfNames), "Tile surface") - mrUser) < 0) return TRUE;
            switch (gArtEd.mode) {
                case kArtEdModeTile2D:
                    artedSurfaceSet(nVTile, i);
                    artedArtDirty(nVTile, kDirtyDat);
                    break;
                default:
                    j = artedBatchProc(artedSurfaceSet, i, kDirtyDat);
                    scrSetMessage("Set %s surface for %d tiles.", surfNames[i], j);
                    break;
            }
            return TRUE;
        case KEY_F7:
            if (!canEdit()) break;
            else if ((i = showButtons(animNames, LENGTH(animNames), "Animation type") - mrUser) < 0) return TRUE;
            else artedAnimTypeSet(nTile, i);
            artedArtDirty(nTile, kDirtyArt);
            scrSetMessage("Tile#%d animation type is %s.", nTile, animNames[panm[nTile].type]);
            return TRUE;
        case KEY_F8:
            while ((i = (showButtons(paintMenu, LENGTH(paintMenu), "Select action") - mrUser)) >= 0)
            {
                switch ( i ) {
                    case 0:
                        if ((j = artedDlgPaint(nTile)) < 0) continue;
                        else scrSetMessage("%d tiles painted.", j);
                        break;
                    case 1:
                        artedDlgReplaceColor(nVTile);
                        artedArtDirty(nTile, kDirtyArt);
                        break;
                }

                break;
            }
            return TRUE;
        case KEY_BACKSPACE:
            if (!canEdit()) break;
            else if (!gArtEd.asksave) return TRUE;
            else if ((i = artedDlgSelectTilePart("Select changes to revert")) <= 0) return TRUE;
            else if ((j = hgltTileCount()) > 1 && tileInHglt(nVTile))
            {
                if (!Confirm("Revert selected changes for %d tiles?", j))
                    return TRUE;
            }
            artedBatchProc(artedRevertTile, i, kDirtyNone);
            scrSetMessage("Changes reverted in selected tiles.");
            return TRUE;
        case KEY_PADPLUS:
        case KEY_PADMINUS:
            if (!canEdit()) break;
            i = (key == KEY_PADMINUS) ? -1 : 1;
            if (ctrl) panm[nTile].type+=i;
            else if (!shift) panm[nTile].frames = ClipRange(panm[nTile].frames + i, 0, 31);
            else panm[nTile].speed = ClipRange(panm[nTile].speed + i, 0, 15);
            scrSetMessage("Tile #%d animation: Type=%s, Frames=%d, Delay=%d.", nTile,
                animNames[panm[nTile].type], panm[nTile].frames, panm[nTile].speed);
            artedArtDirty(nTile, kDirtyPicanm);
            return TRUE;
        case KEY_PAD0:
            if (!canEdit()) break;
            panm[nTile].type   = 0;
            panm[nTile].frames = 0;
            panm[nTile].speed  = 0;
            artedArtDirty(nTile, kDirtyArt);
            scrSetMessage("Reset animation for tile #%d.", nTile);
            return TRUE;
        case KEY_PLUS:
        case KEY_MINUS:
        case KEY_0:
            if (!canEdit()) break;
            i = (key == KEY_PLUS) ? -1 : (key == KEY_0) ? 0 : 1;
            sprintf(buffer2, (i < 0) ? "less" : (i == 0) ? "reset" : "more");
            switch (gArtEd.mode) {
                case kArtEdModeTile2D:
                    if (i == 0) artedShadeSet(nTile, 0);
                    else arttedShadeIterate(nTile, i);
                    scrSetMessage("Tile #%d shade: %+00d",  nTile, tileShade[nTile]);
                    break;
                default:
                    j = artedBatchProc((i == 0) ? artedShadeSet : arttedShadeIterate, i);
                    scrSetMessage("%s shade for %d tiles.", buffer2, j);
                    break;
            }
            artedArtDirty(nTile, kDirtyDat);
            return TRUE;
        case KEY_DELETE:
            if (!canEdit()) break;
            else if (shift) j = artedBatchProc(artedEraseTileInfo, 0, kDirtyPicanm | kDirtyDat);
            else if (ctrl) j = artedBatchProc(artedEraseTileFull, 0, kDirtyAll);
            else j = artedBatchProc(artedEraseTileImage, 0, kDirtyArt);
            scrSetMessage("%d tiles erased.", j);
            return TRUE;
        case KEY_R:
        case KEY_X:
        case KEY_Y:
            if (!canEdit()) break;
            j = 0; //artedPurgeUnchanged();
            switch (key) {
                case KEY_R:
                    if (gArtEd.mode == kArtEdModeTile2D) artedRotateTile(nVTile, 1);
                    else j = artedBatchProc(artedRotateTile, 1, kDirtyArt);
                    sprintf(buffer, "rotated");
                    break;
                case KEY_X:
                    if (gArtEd.mode == kArtEdModeTile2D) artedFlipTileX(nVTile, 1);
                    else j = artedBatchProc(artedFlipTileX, 1, kDirtyArt);
                    sprintf(buffer, "x-flipped");
                    break;
                case KEY_Y:
                    if (gArtEd.mode == kArtEdModeTile2D) artedFlipTileY(nVTile, 1);
                    else j = artedBatchProc(artedFlipTileY, 1, kDirtyArt);
                    sprintf(buffer, "y-flipped");
                    break;
            }
            if (!j) return TRUE;
            scrSetMessage("%d tiles are %s.", j, buffer);
            artedArtDirty(nVTile, kDirtyArt);
            return TRUE;
        case KEY_V:
            if (!ctrl || !canEdit()) break;
            else if ((i = hgltTileCount()) < 1) sprintf(buffer, "You must highlight some tiles.");
            else if ((i = artedDlgCopyTiles(nVTile, i)) < 0) return TRUE;
            else sprintf(buffer, "%d tiles has been copied.", i);
            scrSetMessage(buffer);
            return TRUE;
    }

    return FALSE;

}

void artedInit() {

    keyClear();

    short sect; int i;
    updatesector(posx, posy, &sect);
    gTool.cantest = (sect >= 0 && numsectors > 0);

    toolGetResTableValues();
    toolSetOrigin(&gArtEd.origin, xdim >> 1, ydim >> 1);

    if (gTool.cantest)
    {
        i = getHighlightedObject();
        gTool.objType  = (char)searchstat;
        gTool.objIndex = searchindex;
    }

    memset(tilePluIndex, kPlu0, sizeof(tilePluIndex));
    gTileView.palookup = kPlu0;

    if (!gArtEd.standalone)
    {
        gArtEd.modeMin  = kArtEdModeNone;
        gArtEd.modeMax  = kArtEdModeMax;
    }
}

void artedUninit() {

    keyClear();
    gTool.objType = -1;
    gTool.objIndex = -1;
    gTool.pSprite = NULL;
    gTool.cantest = 0;
    gArtEd.mode = kArtEdModeNone;
    gArtEd.standalone = FALSE;
}

void artedStart(char* filename, BOOL standalone) {

    int i, nPic, type = OBJ_ALL;
    gArtEd.standalone = standalone;

    artedInit();
    gArtEd.modeMin = (gArtEd.standalone) ? kArtEdModeBatch : kArtEdModeNone;
    gArtEd.modeMax = kArtEdModeMax;
    gArtEd.mode    = gArtEd.modeMin;

    if (gTool.cantest) // the map is loaded
    {
        switch (gTool.objType) {
            case OBJ_FLOOR:
                nPic = sector[gTool.objIndex].floorpicnum;
                break;
            case OBJ_CEILING:
                nPic = sector[gTool.objIndex].ceilingpicnum;
                break;
            case OBJ_SPRITE:
                nPic = sprite[gTool.objIndex].picnum;
                break;
            case OBJ_WALL:
                nPic = wall[gTool.objIndex].picnum;
                break;
            case OBJ_MASKED:
                nPic = wall[gTool.objIndex].overpicnum;
                break;
        }
    }
    else if (filename) // editing ART files directly from command line
    {
        // !!!
        // right now you can only edit default game
        // tiles or the ones that are defined in the core ini
        for (i = 0; i < nArtFiles && stricmp(gArtFiles[i].path, filename) != 0; i++);
        if (i >= nArtFiles)
        {
            getFilename(filename, buffer, TRUE);
            Alert("\"%s\" is not exist nor defined ART file.", strupr(buffer));
            return;
        }

        nPic = gArtFiles[i].start;
    }

    nPic = ClipRange(nPic, 0, gMaxTiles);
    while( 1 )
    {
        tilePick(nPic, -1, type, gArtEdModeNames[gArtEd.mode]);
        if (filename && gArtEd.asksave)
        {
            switch(YesNoCancel("Save ART changes?")) {
                case mrOk:
                    artedSaveChanges();
                    // no break
                case mrNo:
                    break;
                default:
                    continue;
            }
        }
        break;
    }

    artedUninit();


}

void artedPurgeUnchanged() {

    for (int i = 0; i < gMaxTiles; i++)
    {
        if (gDirtTiles[i] & kDirtyArt) continue;
        tilePurgeTile(i);
    }

}

BOOL artedPaint(int nTile, PALETTE pal) {

    if (!tileLoadTile(nTile)) return FALSE;
    remapColors(waloff[nTile], tilesizx[nTile]*tilesizy[nTile], pal);
    return TRUE;
}

int artedDlgSelPal(char* title, char* path, PALETTE out) {

    char types[256];
    char* file, *errMsg;
    int i;

    // concatenate all supported palette file extensions for dir explorer
    array2str(gSuppPalettes, LENGTH(gSuppPalettes), types, sizeof(types));

    while ( 1 )
    {
        if ((file = browseOpenFS(path, types, title)) == NULL) return -1;
        else if ((errMsg = retnCodeCheck(i = palLoad(file, out), gPalErrorsCommon)) != NULL)
        {
            getFilename(file, buffer, TRUE);
            Alert("Error %d in %s: %s", abs(i), buffer, errMsg);
            continue;
        }
        break;
    }

    return 0;
}



int artedDlgPaint(int nTile) {

    PALETTE pal;
    int i, j, cnt;

    cnt = hgltTileCount();
    if (artedDlgSelPal("Paint tiles", gPaths.palPaint, pal) < 0)
        return -1;

    if (cnt <= 1 || !tileInHglt(nTile))
    {
        if (artedPaint(nTile, pal))
        {
            artedArtDirty(nTile, kDirtyArt);
            j = 1;
        }
    }
    else
    {
        for (i = 0, j = 0; i < kMaxTiles; i++)
        {
            if (!tileInHglt(i) || !artedPaint(i, pal)) continue;
            artedArtDirty(i, kDirtyArt);
            j++;
        }
    }

    return j;
}

void artedFlipTileY(int nTile, int ofs) {

    BYTE* pTile;
    if ((pTile = tileLoadTile(nTile)) == NULL)
        return;

    char saved;
    int col, i, j, k, f;
    int wh = tilesizx[nTile];
    int hg = tilesizy[nTile];
    int len = wh*hg;

    for (col = i = 0; i < wh; i++, col+=hg)
    {
        for (j = 0; j < hg >> 1; j++)
        {
            k = col + j;
            f = col + hg - j - 1;
            saved = pTile[k];
            pTile[k] = pTile[f];
            pTile[f] = saved;
        }
    }

    if (ofs)
    {
        if (panm[nTile].ycenter > 0)
            panm[nTile].ycenter = -panm[nTile].ycenter;
        else if(panm[nTile].ycenter < 0)
            panm[nTile].ycenter = abs(panm[nTile].ycenter);
    }
}

void artedFlipTileX(int nTile, int ofs) {

    BYTE* pTile;
    if ((pTile = tileLoadTile(nTile)) == NULL)
        return;

    int i, j, wh = tilesizx[nTile], hg = tilesizy[nTile];
    int len = wh*hg;

    for (i = 0; i < len >> 1; i++)
    {
        j = pTile[i];
        pTile[i] = pTile[len - i - 1];
        pTile[len - i - 1] = (char)j;
    }

    artedFlipTileY(nTile, FALSE);

    if (ofs)
    {
        if (panm[nTile].xcenter > 0)
            panm[nTile].xcenter = -panm[nTile].xcenter;
        else if(panm[nTile].xcenter < 0)
            panm[nTile].xcenter = abs(panm[nTile].xcenter);
    }

}

void artedRotateTile(int nTile, int ofs) {

    BYTE *pTile, *pTemp;
    if ((pTile = tileLoadTile(nTile)) == NULL)
        return;

    int i = 0, j = 0, k = 0, f;
    int wh = tilesizx[nTile];
    int hg = tilesizy[nTile];
    int len = wh*hg;

    PICANM backup = panm[nTile];
    pTemp = (BYTE*)Resource::Alloc(len);
    memcpy(pTemp, pTile, len);

    tileFreeTile(nTile);
    pTile = tileAllocTile(nTile, hg, wh);

    while (i < len && k < len)
    {
        pTile[i] = pTemp[j]; // ROW of the tile
        k = i, f = j;
        while ((f += hg) < len) // COLS of the tile from top to bottom
        {
            if (++k >= len)
                break;

            pTile[k] = pTemp[f];
        }

        i+=wh, j++;
    }

    panm[nTile] = backup;
    if (ofs)
    {
        i = panm[nTile].xcenter;
        panm[nTile].xcenter = panm[nTile].ycenter;
        panm[nTile].ycenter = i;
    }

    Resource::Free(pTemp);
    artedFlipTileY(nTile, ofs);

}

int artedDrawTile(int nTile, int nOctant, int nShade)
{
    char flags = kRSNoClip; int ang = 0;
    nTile = toolGetViewTile(nTile, nOctant, &flags, &ang);
    if (keystatus[KEY_CAPSLOCK]) flags |= kRSNoMask;
    if (nTile == 2342)
        FireProcess();

    rotatesprite(gArtEd.origin.x << 16, gArtEd.origin.y << 16, gTool.tileZoom, ang, nTile, (char)nShade,
                gTileView.palookup, flags, 0, 0, xdim-1, ydim-1);

    return nTile;
}

void artedArtDirty(int nTile, char what)
{
    artedGetFile(nTile);
    dassert(tilefilenum[nTile] != 255);
    gArtFiles[tilefilenum[nTile]].dirty = 1;
    gDirtTiles[nTile]   |= what;
    gArtEd.asksave      |= what;
}

int artedArray2Art(char* fname, short* array, int len, int startID, char flags, PALETTE pal) {

    uint32_t blk[4]; char blkcnt = 0;
    int i, j, k, hFile, sixofs, siyofs, pnmofs, datofs; PICANM pnm;
    if ((hFile = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE)) < 0)
        return -1;

    // standard art file header
    i = 0x0001;
    write(hFile, &i, sizeof(int32_t));
    write(hFile, &len, sizeof(int32_t));
    write(hFile, &startID, sizeof(int32_t)); i = startID + len - 1;
    write(hFile, &i, sizeof(int32_t));

    // standard art data
    sixofs = tell(hFile);                       // tilesizx[] start offset
    siyofs = sixofs + (sizeof(int16_t)*len);    // tilesizy[] start offset
    pnmofs = siyofs + (sizeof(int16_t)*len);    // panm[] start offset
    datofs = pnmofs + (sizeof(PICANM)*len);     // waloff[] start offset

    for (i = 0; i < len; i++)
    {
        j = array[i];
        lseek(hFile, sixofs, SEEK_SET);
        sixofs+=write(hFile, &tilesizx[j], sizeof(int16_t));

        lseek(hFile, siyofs, SEEK_SET);
        siyofs+=write(hFile, &tilesizy[j], sizeof(int16_t));

        memcpy(&pnm, &panm[j], sizeof(PICANM));
        pnm.view = viewType[j], pnm.filler = 0;

        lseek(hFile, pnmofs, SEEK_SET);
        pnmofs+=write(hFile, &pnm, sizeof(PICANM));

        lseek(hFile, datofs, SEEK_SET);
        if ((k = tilesizx[j]*tilesizy[j]) <= 0)
            continue;

        datofs+=write(hFile, tileLoadTile(j), k);
    }

    //////////
    // add READ-compatible file footer with various additional info
    //////////////////////
    XRTBLK_HEAD head;
    if (pal)
    {
        XRTBLKb_DATA data;
        blk[blkcnt++] = tell(hFile);
        head.blktype = kArtXBlockGlobalPalette;         // art file palette
        head.headsiz = sizeof(head);                    // total size of this block header
        head.datasiz = sizeof(data);                    // total size of this block data

        memcpy(data.artPal, pal, sizeof(data.artPal));
        write(hFile, &head, head.headsiz);              // write header
        write(hFile, &data, sizeof(data));              // write data
    }

    if (flags & kArtSaveFlagTileInfo)
    {
        XRTBLKa_DATA data;
        blk[blkcnt++] = tell(hFile);
        head.blktype  = kArtXBlockTileMixed;        // tilenum, shade, surface, voxelID
        head.headsiz  = sizeof(head);               // total size of this block header
        head.datasiz  = (ushort)(sizeof(data)*len); // total size of this block data

        write(hFile, &head, head.headsiz);          // write header

        for (i = 0; i < len; i++)                   // write data for each tile
        {
            j = array[i];
            data.number  = (ushort)j;
            data.shade   = tileShade[j];
            data.surface = surfType[j];
            data.voxelID = voxelIndex[j];

            write(hFile, &data, sizeof(data));
        }
    }

    // attach block map
    if (blkcnt)
    {
        // write relative offsets to headers of blocks
        for (i = 0; i < blkcnt; i++)
        {
            j = blk[i] - tell(hFile);
            write(hFile, &j, sizeof(blk[0]));
        }

        // write total count of blocks just before map signature
        write(hFile, &blkcnt, sizeof(blkcnt));

        // write block map signature
        // you can just read strlen(kArtBlockMapSign) bytes from
        // end of the file to ensure that block map is present and
        // go through offsets to read additional info by determining
        // block type in it's header.
        write(hFile, buffer, sprintf(buffer, kArtBlockMapSign));
    }

    close(hFile);
    return 0;
}

void artedSaveArtFiles() {

    BOOL error = FALSE;
    int nTiles, i, j;
    char tempName[256], fileName[256];
    short* array = NULL;

    for (i = nArtFiles; i >= 0 ; i--)
    {
        ARTFILE* artFile =& gArtFiles[i];
        nTiles = artFile->end - artFile->start + 1;
        if (!artFile->dirty || nTiles <= 0) continue;
        else if (!tmpFileMake(tempName))
        {
            artedLogMsg("Failed to create temporary file for %s!", artFile->path);
            continue;
        }

        sprintf(fileName, artFile->path);
        array = (short*)Resource::Alloc(sizeof(short)*nTiles);
        for (j = 0; j < nTiles; j++)
            array[j] = (short)(artFile->start + j);

        error = (BOOL)(artedArray2Art(tempName, array, nTiles, artFile->start) != 0);
        Resource::Free(array);

        fileAttrSetWrite(fileName);
        if (error || !fileDelete(fileName) || rename(tempName, fileName) != 0)
        {
            artedLogMsg("Failed to save %s (temp: %s)!", fileName, tempName);
            Alert("Failed to save %s!", fileName);
            continue;
        }

        artFile->dirty = 0;
    }
}

void artedSaveDatFiles() {

    int i, hFile, cnt = 1;
    if (!(gArtEd.asksave & kDirtyDat))
        return;

    // don't create SHADE.DAT if there is no shaded tiles
    if (fileExists(gPaths.shadeDAT))
        unlink(gPaths.shadeDAT);

    for (i = 0; i < kMaxTiles && !tileShade[i]; i++);

    if (i >= kMaxTiles) cnt++;
    else if (FileSave(gPaths.shadeDAT, tileShade, sizeof(tileShade)))    cnt++;
    if (FileSave(gPaths.voxelDAT, voxelIndex, sizeof(voxelIndex)))       cnt++;
    if (FileSave(gPaths.surfDAT, surfType, sizeof(surfType)))            cnt++;
    if (cnt >= 3)
        gArtEd.asksave &= ~kDirtyDat;

}

BOOL tileInHglt(int nTile) { return (BOOL)gHgltTiles[nTile].ok; }
BOOL isSysTile(int nTile) { return (BOOL)gSysTiles.isBusy(nTile); }
void artedSurfaceSet(int nTile, int value) { surfType[nTile] = (BYTE)value; }
void artedShadeSet(int nTile, int value) { tileShade[nTile] = (schar)ClipRange(value, -127, NUMPALOOKUPS(1)); }
void arttedShadeIterate(int nTile, int value) {
    int val = tileShade[nTile];
    artedShadeSet(nTile, val + value);
}

void artedAnimTypeSet(int nTile, int value) {

    panm[nTile].type = value;
    if (panm[nTile].type > 0)
    {
        if (!panm[nTile].frames) panm[nTile].frames = 1;
        if (!panm[nTile].speed)  panm[nTile].speed  = 3;
    }

}

void artedEraseTileImage(int nTile, int) {

    tilePurgeTile(nTile);
    tilesizx[nTile]         = 0;
    tilesizy[nTile]         = 0;
    picsiz[nTile]           = 0;
    hgltTileRem(nTile);
}

void artedEraseTileInfo(int nTile, int) {

    surfType[nTile]         = 0;
    voxelIndex[nTile]       = -1;
    tileShade[nTile]        = 0;
    memset(&panm[nTile], 0, sizeof(PICANM));

}

void artedEraseTileFull(int nTile, int) {

    artedEraseTileImage(nTile, 0);
    artedEraseTileInfo(nTile, 0);

}

int artedDlgViewTypeSet(int nTile) {

    int i, nView;

    switch (viewType[nTile]) {
        case kSprViewVox:
        case kSprViewVoxSpin:
            if ((i = showButtons(viewMenu, LENGTH(viewMenu), "Select action") - mrUser) < 0) return -1;
            else if (i == 1)
            {
                artedDlgSelectVoxel(nTile);
                return -1;
            }
            break;
    }

    if ((nView = showButtons(viewNames, LENGTH(viewNames), "Tile view type") - mrUser) < 0)
        return -1;

    switch (nView) {
        case kSprViewVox:
        case kSprViewVoxSpin:
            if (!rngok(voxelIndex[nTile], 0, kMaxVoxels))
            {
                if (Confirm("Do you want to set voxel ID now?"))
                    artedDlgSelectVoxel(nTile);
            }
            // no break
        default:
            viewType[nTile] = (char)nView;
            if (!isExternalModel(nTile)) panm[nTile].view = nView;
            break;

    }

    artedArtDirty(nTile, kDirtyPicanm);
    return viewType[nTile];
}

int artedDlgSelectVoxel(int nTile) {

    int i = 0;
    while ( 1 )
    {
        i = GetNumberBox("Enter RFF Voxel ID", voxelIndex[nTile], -1);
        if (i < -1 || i >= kMaxVoxels) Alert("Correct voxel ID range is: %d - %d", -1, kMaxVoxels - 1);
        else if (i == -1 || (i >= 0 && gSysRes.Lookup(i, "KVX")) || Confirm("Voxel #%d is not found! Continue anyway?", i))
        {
            voxelIndex[nTile] = (short)i;
            artedArtDirty(nTile, kDirtyDat);
            break;
        }
    }

    return i;

}

int artedDlgSaveSelection(int nTile) {

    int i, j, k, retn = -1;
    char* filename = NULL; short* hglt = NULL;
    PALETTE pal;
    
    sprintf(buffer, "Save as...");
    if ((k = showButtons(exportMenu, LENGTH(exportMenu), buffer)) < mrUser)
        return -1;

    hglt = (short*)Resource::Alloc(sizeof(short)*kMaxTiles);
    for (i = 0, j = 0; i < kMaxTiles; i++)
    {
        if (tileInHglt(i))
            hglt[j++] = (short)i;
    }

    if (j <= 0)
        hglt[j++] = (short)nTile;

    switch (k-=mrUser) {
        case 0:
            sprintf(buffer, "Save %d tiles as...", j);
            while( 1 )
            {
                if ((filename = browseSave(gPaths.images, ".art", buffer)) == NULL)
                {
                    if (Confirm("Cancel saving?"))
                        break;
                    
                    continue;
                }
                
                plsWait();
                memcpy(pal, gamepal, sizeof(gamepal)); retn = j;
                if (Confirm("Save extra art information as well?"))
                {
                    artedArray2Art(filename, hglt, j, 0, 0xFF, pal);
                }
                else
                {
                    artedArray2Art(filename, hglt, j, 0, 0x00, NULL);
                }
                
                break;
            }
            break;
        case 1:
            for (i = 0, retn = 0, k = 0; i < j; i++)
            {
                sprintf(buffer, "TIL%04d.TGA", hglt[i]);
                sprintf(buffer2, "Saving tile #%d in %s (%d of %d)", hglt[i], buffer, ++k, j);
                splashScreen(buffer2);
                if (tile2tga(buffer, hglt[i]) == 0)
                    retn++;
            }
            break;
#ifdef I2TDEBUG
        case 2:
            for (i = 0, retn = 0, k = 0; i < j; i++)
            {
                sprintf(buffer, "TIL%04d.QBM", hglt[i]);
                sprintf(buffer2, "Saving tile #%d in %s (%d of %d)", hglt[i], buffer, ++k, j);
                splashScreen(buffer2);
                if (tile2qbm(buffer, hglt[i]) == 0)
                    retn++;
            }
            break;
#endif
/*      case 3:
            for (i = 0, retn = 0, k = 0; i < j; i++)
            {
                sprintf(buffer, "TIL%04d.PCX", hglt[i]);
                sprintf(buffer2, "Saving tile #%d in %s (%d of %d)", hglt[i], buffer, ++k, j);
                splashScreen(buffer2);
                scrNextPage();

                if (tile2pcx(buffer, hglt[i]) == 0)
                    retn++;
            }
            break; */
    }

    Resource::Free(hglt);
    return retn;
}


int artedDlgImportArt(int nTile) {

    PICANM pnm;
    PALETTE pal, palbck;
    int i, j, idx = -1, err = 0, hFile, start, end, tiles, sixofs, siyofs, pnmofs, datofs;
    short tsx = -1, tsy = -1, noExtra = -1, noBlank = -1;

    XRTBLK_HEAD head; XRTBLKa_DATA tileInfo; XRTBLKb_DATA customPal;
    int bcnt, mapofs, blkofs, infofs;

    while( 1 )
    {
        if (artedDlgSelPal("Load palette of ART file(s)", gPaths.palImport, pal) < 0) return -1;
        else if (browseOpenMany(gPaths.images, ".art", "Import ART file(s)") == NULL) continue;
        else break;
    }

    artedPurgeUnchanged();
    palFixTransparentColor(pal);
    memcpy(palbck, pal, sizeof(pal));   // backup the palette

    j = dirBrowseCountSelected(), i = 0;
    while(dirBrowseEnumSelected(&idx, buffer3))
    {
        infofs = 0;                         // restore extra tile info for next file
        memcpy(pal, palbck, sizeof(pal));   // restore palette for next file
        getFilename(buffer3, buffer, TRUE);
        sprintf(buffer2, "Processing %s (%d of %d)", buffer, ++i, j);
        splashScreen(buffer2);
        if ((hFile = open(buffer3, O_BINARY | O_RDONLY)) < 0) continue;

        err = readArtHead(hFile, &start, &end, &sixofs, &siyofs, &pnmofs, &datofs);
        if (!err && ((tiles = end - start + 1) <= 0))
            err = -1;

        if (err < 0)
        {
            char* errMsg = retnCodeCheck(err, gArtFileErrors);
            Alert("Error #%d in %s: %s", abs(err), strupr(buffer), errMsg);
            close(hFile);
            continue;
        }


        // some extra info in this file?
        if (xartOffsets(hFile, &bcnt, &mapofs))
        {
            if (noExtra == -1)
            {
                noExtra = (Confirm("Import extra ART info as well?")) ? 0 : 1;
                showframe();
            }

            if (noExtra == 0)
            {
                while(bcnt--)                                                   // read all the block offsets from the block map
                {
                    mapofs+=read(hFile, &blkofs, sizeof(int));                  // read block offset value
                    lseek(hFile, blkofs-sizeof(int), SEEK_CUR);                 // go to the actual block
                    read(hFile, &head, sizeof(head));                           // read block header
                    switch (head.blktype) {
                        case kArtXBlockTileMixed:                               // DAT tile info block
                            if (head.datasiz != sizeof(tileInfo)*tiles) break;  // check for consistency
                            infofs = tell(hFile);                               // info start offset
                            break;
                        case kArtXBlockGlobalPalette:                           // custom palette block
                            if (head.datasiz != sizeof(customPal)) break;       // check for consistency
                            read(hFile, pal, sizeof(pal));                      // using custom palette now
                            palFixTransparentColor(pal);                        // try to fix 255's
                            break;
                    }

                    lseek(hFile, mapofs, SEEK_SET);                             // back to the block map
                }
            }
        }

        while(tiles-- > 0)
        {
            while(nTile < gMaxTiles && isSysTile(nTile))
                nTile++;

            if (nTile >= gMaxTiles)
            {
                Alert("Max tiles reached!");
                close(hFile);
                return -1;
            }
            else if (!artedGetFile(nTile))
            {
                Alert("No ART file for tile #%d!", nTile);
                close(hFile);
                return -1;
            }

            lseek(hFile, sixofs, SEEK_SET);
            sixofs+=read(hFile, &tsx, sizeof(short));

            lseek(hFile, siyofs, SEEK_SET);
            siyofs+=read(hFile, &tsy, sizeof(short));

            lseek(hFile, pnmofs, SEEK_SET);
            pnmofs+=read(hFile, &pnm, sizeof(PICANM));

            if (tsx*tsy <= 0)
            {
                if (noBlank == -1)
                {
                    noBlank = (Confirm("Skip blank tiles?")) ? 1 : 0;
                    showframe();
                }

                if (noBlank ==  1)
                {
                    if (infofs)
                        infofs += sizeof(tileInfo);

                    continue;
                }
            }

            artedEraseTileFull(nTile);
            artedArtDirty(nTile, kDirtyArt | kDirtyPicanm | kDirtyDat);
            if (tsx*tsy)
                tileAllocTile(nTile, tsx, tsy);

            panm[nTile].frames      = pnm.frames;
            panm[nTile].update      = pnm.update;
            panm[nTile].type        = pnm.type;
            panm[nTile].xcenter     = pnm.xcenter;
            panm[nTile].ycenter     = pnm.ycenter;
            panm[nTile].speed       = pnm.speed;
            panm[nTile].filler      = 0;
            viewType[nTile]         = (BYTE)pnm.view;
            if (!isExternalModel(nTile))
                panm[nTile].view = viewType[nTile];

            if (waloff[nTile])
            {
                lseek(hFile, datofs, SEEK_SET);
                datofs += read(hFile, (void*)waloff[nTile], tsx*tsy);
                if (memcmp(pal, gamepal, sizeof(pal)) != 0)
                    remapColors(waloff[nTile], tsx*tsy, pal);
            }

            if (infofs) // read extra tile info
            {
                lseek(hFile, infofs, SEEK_SET);
                infofs += read(hFile, &tileInfo, sizeof(tileInfo));

                tileShade[nTile]        = tileInfo.shade;
                surfType[nTile]         = tileInfo.surface;
                voxelIndex[nTile]       = tileInfo.voxelID;
            }

            nTile++;
        }

        close(hFile);

    }

    return 0;

}

int artedDlgImportFLOORS(int nTile)
{
    #define kFileSize 352320
    #define kPicSiz 64
    #define kNumFloors 64
    #define kSkipSiz (32*32)+(16*16)+(8*8)+64

    int i = 0, j = 0, idx = -1, retn = 0;
    int hFile, nFloors, nTileTemp, siz = kPicSiz*kPicSiz, noDuplicates = -1;
    char defaultPal[_MAX_PATH], tmp[512]; BYTE *pTemp, *pTile; PALETTE pal;

    sprintf(defaultPal, "%s\\chasm.pal", kPalImportDir);
    while(i < 80)
        j += sprintf(&tmp[j], ".%02d", ++i);

    i = palLoad(defaultPal, pal);
    while( 1 )
    {
        if (i < 0 && artedDlgSelPal("Load palette", gPaths.palImport, pal) < 0) return -1;
        else if (browseOpenMany(gPaths.images, tmp, "Import FLOOR file(s)") != NULL) break;
        else if (i >= 0)
            return -1;
    }

    palShift(pal);
    nTileTemp = tileGetBlank();
    j = dirBrowseCountSelected(), i = 0;
    while(retn >= 0 && dirBrowseEnumSelected(&idx, buffer3))
    {
        getFilename(buffer3, buffer, TRUE);
        sprintf(buffer2, "Processing %s (%d of %d)", buffer, ++i, j);
        splashScreen(buffer2);

        if ((hFile = open(buffer3, O_BINARY|O_RDONLY, S_IREAD|S_IWRITE)) < 0) continue;
        else if (filelength(hFile) != kFileSize)
        {
            Alert("\"%s\" is incorrect or not Chasm FLOOR file.", buffer);
            close(hFile);
            continue;
        }

        // skip header
        lseek(hFile, kNumFloors, SEEK_CUR);

        nFloors = kNumFloors;
        while(nFloors--)
        {
            while(nTile < gMaxTiles && isSysTile(nTile))
                nTile++;

            if (nTile >= gMaxTiles)
            {
                Alert("Max tiles reached!");
                retn = -3;
                break;
            }
            else if (!artedGetFile(nTile))
            {
                Alert("No ART file for tile #%d!", nTile);
                retn = -3;
                break;
            }

            pTemp = tileAllocTile(nTileTemp, kPicSiz, kPicSiz);

            // read 64x64 texture
            read(hFile, pTemp, siz);

            // skip texture mips and appendix
            lseek(hFile, kSkipSiz, SEEK_CUR);

            // unused entries always have single color, so skip it
            if (countUniqBytes(pTemp, siz) > 1)
            {
                artedRotateTile(nTileTemp);
                remapColors((intptr_t)pTemp, siz, pal);
                if (tileExists(pTemp, kPicSiz, kPicSiz) >= 0)
                {
                    if (noDuplicates == -1)
                        noDuplicates = (Confirm("Skip duplicates?")) ? 1 : 0;

                    if (noDuplicates == 1)
                        continue;
                }

                artedEraseTileImage(nTile);
                artedCopyTile(nTileTemp, nTile);
                artedArtDirty(nTile, kDirtyArt | kDirtyPicanm | kDirtyDat);
                nTile++, retn++;
            }
        }

        close(hFile);
    }

    tileFreeTile(nTileTemp);
    return retn;
}

int artedDlgImportImage(int nTile) {

    BOOL asked = FALSE;
    char* img; char *errorMsg = NULL;
    char types[256], tmp[256];
    memset(types, 0, sizeof(types));
    int i, idx = -1, images = 0, errCode = 0, j = 0;

    // concatenate all supported image file extensions for dir explorer
    array2str(gSuppImages, LENGTH(gSuppImages), types, sizeof(types));

    while ( 1 )
    {
        errCode = 0;
        if ((img = browseOpenMany(gPaths.images, types, "Import images")) == NULL)
            return -1;

        i = dirBrowseCountSelected();

        if (!asked)
        {
            asked = TRUE;
            for(j = 0; j < i; j++)
            {
                if (nTile + j >= gMaxTiles) break;
                else if (tileLoadTile(nTile + j))
                {
                    if (!Confirm("Replace existing tiles?")) return -1;
                    break;
                }
            }
        }

        artedPurgeUnchanged();

        j = 0;
        while ( 1 )
        {
            errorMsg = NULL;
            if (!dirBrowseEnumSelected(&idx, tmp))
                return images;

            while(nTile < gMaxTiles && isSysTile(nTile))
                nTile++;

            if (nTile >= gMaxTiles)
            {
                Alert("Max tiles reached!");
                return images;
            }
            else if (!artedGetFile(nTile))
            {
                Alert("Failed to get an ART file for tile #%d!", nTile);
                return images;
            }

            getFilename(tmp, buffer, TRUE);
            sprintf(buffer2, "Processing: %s (%d of %d)", buffer, ++j, i);
            splashScreen(buffer2);

            IMG2TILEFUNC pFunc = imgGetConvFunc(imgGetType(tmp));
            errCode = (pFunc) ? imgFile2TileFunc(pFunc, tmp, nTile) : -5;

            if (errCode < 0)
            {
                if (!errorMsg) errorMsg = retnCodeCheck(errCode, gImgErrorsCommon);
                sprintf(buffer2, "Error %d in %s: %s.", abs(errCode), buffer, errorMsg);

                if (j < i)
                {
                    strcat(buffer2, " ");
                    strcat(buffer2, "Continue?");
                    if (!Confirm(buffer2))
                        return images;
                }
                else
                {
                    Alert(buffer2);
                    if (i > 1) return images;
                    break;
                }
            }
            else
            {
                artedArtDirty(nTile, kDirtyArt);
                nTile++, images++;
            }
        }
    }
}

char* artedGetFile(int nTile) {

    if (tilefilenum[nTile] != 255)
        return gArtFiles[tilefilenum[nTile]].path;

    int tiles = 0;
    int i, j, c = 0;
    int fnum1 = -1, fnum2 = -1;
    int tile1 = -1, tile2 = -1;
    BOOL maxReached = FALSE;

    // search to the start
    for (tile1 = nTile; tile1 >= 0; tile1--)
    {
        if (tilefilenum[tile1] == 255) continue;
        fnum1 = tilefilenum[tile1];
        break;
    }

    // search to the end
    for (tile2 = nTile; tile2 < kMaxTiles; tile2++)
    {
        if (tilefilenum[tile2] == 255) continue;
        fnum2 = tilefilenum[tile2];
        break;
    }

    // no art files found
    // create new ones using kTPF alignment
    if (fnum1 < 0 && fnum2 < 0)
    {
        c = (nTile / kTPF) + 1;
        sprintf(gMisc.tilesBaseName, "TILES");  // use registered tiles name
        for(i = 0; i < c; i++)
        {
            ARTFILE* file =& gArtFiles[i];
            file->dirty = 1; file->start = file->end = tiles;
            sprintf(file->path, "%s%03d.ART", gMisc.tilesBaseName, i);
            for (j = tiles; j <= tiles + kTPFS; j++)
                tilefilenum[file->end++] = i;

            tiles+=kTPF, nArtFiles++;
        }

        gArtEd.asksave |= kDirtyArt;
        return artedGetFile(nTile);
    }

    // no file between existing files
    // grow previously found ART file
    if (fnum1 >= 0 && fnum2 >= 0)
    {
        dassert(fnum1 < fnum2);
        ARTFILE* file1 =& gArtFiles[fnum1];
        ARTFILE* file2 =& gArtFiles[fnum2];

        file1->dirty = 1;
        file1->end = file2->start - 1;
        for (i = file1->start; i <= file1->end; i++)
            tilefilenum[i] = fnum1;

        gArtEd.asksave |= kDirtyArt;
        return artedGetFile(nTile);
    }

    // make new art files AFTER existing ones using kTPF alignment
    if (fnum2 < 0 && fnum1 >= 0)
    {
        c = ((nTile - tile1) / kTPF) + 1;
        for (i = fnum1 + 1; i <= fnum1 + c; i++)
        {
            ARTFILE* file =& gArtFiles[i];
            sprintf(file->path, "%s%03d.ART", gMisc.tilesBaseName, i);
            file->dirty = 1;
            file->start = tile1 + tiles + 1;
            file->end = ClipHigh(file->start + kTPFS, kMaxTiles - 1);
            for (j = file->start; j <= file->end; tiles++, tilefilenum[j++] = i);

            nArtFiles++;
            if (file->end >= kMaxTiles - 1)
                break;
        }

        gArtEd.asksave |= kDirtyArt;
        return artedGetFile(nTile);
    }

    return NULL;

}

int artedReplaceColor(int nTile, BYTE colorA, BYTE colorB)
{
    return replaceByte((BYTE*)waloff[nTile], tilesizx[nTile]*tilesizy[nTile], colorA, colorB);
}


BOOL artedDlgReplaceColor(int nTile) {

    PALETTE pal, tmp;
    BYTE* pTile, *pPreview;
    const int r = 16, c = 16, pad = 4;
    short nPreview, sz, scsp = (short)(ydim >> 7);
    BYTE tileColors[256], bTileColors[256], paletteColors[256];
    char* plu;

    sz = (short)(10 + scsp);

    int x, y, i = 0, j = 0;
    int bw = 50 + (4*scsp), bh = 12 + (4*scsp);
    int pw = (pad) + (c*sz);
    int ph = (pad) + (r*sz);
    int dw = ((pw << 1) + (pad * 5)) - 1;
    int dh = 14 + (ph << 1) + (pad * 5) + bh;
    int psizex = pw, psizey = (ph << 1) + pad, tsize = psizex - 10;
    int twh = tilesizx[nTile], thg = tilesizy[nTile], imgLen = twh*thg;
    char flags = 0x01;

    if ((pTile = tileLoadTile(nTile)) == NULL)
    {
        Alert("Cannot load tile #%d.", nTile);
        return FALSE;
    }
    else if (!tileAllocSysTile(&nPreview, twh, thg))
    {
        Alert("Cannot allocate tile.");
        return FALSE;
    }

    pPreview = (BYTE*)waloff[nPreview];
    memcpy(pPreview, pTile, imgLen);
    tileDrawGetSize(nPreview, tsize, &twh, &thg);

    memcpy(pal, gamepal, sizeof(pal));
    memset(paletteColors, 1, sizeof(paletteColors));
    memset(tileColors, 0, sizeof(tileColors));
    while (i < imgLen) tileColors[pPreview[i++]] = 1;       // get tile colors
    memcpy(bTileColors, tileColors, sizeof(bTileColors));   // backup tile colors
    buffer[0] = '\0';

    while( 1 )
    {
        x = y = pad;
        Window dialog(0, 0, dw, dh, "Replace color");
        Panel* pPanColorA   = new Panel(x + pw + pad, y, pw, ph, -1, 1, 0);
        Panel* pPanColorB   = new Panel(x + pw + pad, y + ph + pad, pw, ph, -1, 1, 0);
        Panel* pPreviewCont = new Panel(x, y, psizex, psizey, -1, 1, 0);

        ColorSelect* pColorPickA = new ColorSelect(1, 1, r, c, sz, tileColors);
        ColorSelect* pColorPickB = new ColorSelect(1, 1, r, c, sz, paletteColors);

        pColorPickB->connect = pColorPickA;

        pPanColorA->Insert(pColorPickA);
        pPanColorB->Insert(pColorPickB);

        pPreviewCont->Insert(new Shape(4, 4, psizex-8, psizey-8, gStdColor[26]));
        pPreviewCont->Insert(new Shape(5, 5, psizex-10, psizey-10, gStdColor[22]));

        pPreviewCont->Insert(new Tile((psizex - twh) >> 1, (psizey - thg) >> 1, nPreview, tsize, tsize, 0, 0x0));
        if (buffer[0])
        {
            pPreviewCont->Insert(new Message(8, 8, buffer, qFonts[1], 15));
            buffer[0] = '\0';
        }
        x = dw - bw - (pad << 1), y = (ph << 1) + (pad * 3);
        TextButton* pCancel = new TextButton(x, y, bw, bh, "&Quit", mrCancel);      x -= (bw + (pad >> 1));
        TextButton* pOkay   = new TextButton(x, y, bw, bh, "&Save", mrOk);          x -= (bw + (pad >> 1));
        TextButton* pApply  = new TextButton(x, y, bw, bh, "&Paint", 100);          x = pad;

        TextButton* pRevert = new TextButton(x, y, bw, bh, "&Reset", 101); x += (bw + (pad >> 1));
        pRevert->fontColor  = 4;

        TextButton* pLoadPal = new TextButton(x, y, bw + 40, bh, "&Apply palette", 102);
        pLoadPal->fontColor = 1;

        if (memcmp(pTile, pPreview, imgLen) == 0)
        {
            pOkay->disabled = TRUE;
            pOkay->canFocus = FALSE;
            pOkay->fontColor = 8;
        }

        dialog.Insert(pPanColorA);
        dialog.Insert(pPanColorB);
        dialog.Insert(pPreviewCont);
        dialog.Insert(pApply);
        dialog.Insert(pOkay);
        dialog.Insert(pCancel);
        dialog.Insert(pRevert);
        dialog.Insert(pLoadPal);

        switch (ShowModal(&dialog)) {
            default:
                continue;
            case mrCancel:
                if (memcmp(pTile, pPreview, imgLen) == 0) break;
                else if ((i = YesNoCancel("Save current colors?")) != mrOk)
                {
                    if (i != mrNo) continue;
                    else memcpy(pPreview, pTile, imgLen);
                    break;
                }
                // no break
            case mrOk:
                while( 1 )
                {
                    if ((i = showButtons(replaceColorSaveMenu, LENGTH(replaceColorSaveMenu), "Save changes") - mrUser) == 0)
                    {
                        if ((plu = browseSave(gPaths.palPaint, ".plu")) == NULL)
                            continue;

                        i = 0;
                        getFilename(plu, buffer3, TRUE);

                        sprintf(buffer2, "from %d to %d", 0, 128);
                        sprintf(buffer, "Gray level (%s)", buffer2);
                        if ((i = GetNumberBox(buffer, i, -1)) < 0) continue;
                        else if (i > 128)
                        {
                            Alert("Value must be %s.", buffer2);
                            continue;
                        }

                        strupr(buffer3);
                        if (!BuildPLU(plu, pal, i))
                        {
                            Alert("Failed to save \"%s\"", buffer3);
                            continue;
                        }

                        j = sprintf(buffer, "Palookup saved as \"%s\".", buffer3);
                        if (i > 0)
                        {
                            buffer[j - 1] = '\0';
                            sprintf(buffer2, "with gray level %d.", i);
                            strcat(buffer, " ");
                            strcat(buffer, buffer2);
                        }

                        strcat(buffer, " ");
                        strcat(buffer, "Apply it to tile(s)?");
                        if (!Confirm(buffer)) break;
                        else if (!tileInHglt(nTile)) artedPaint(nTile, pal);
                        else
                        {
                            for (i = 0; i < gMaxTiles; i++)
                            {
                                if (tileInHglt(i))
                                    artedPaint(i, pal);
                            }
                        }
                    }
                    else if (i == 1)
                    {
                        memcpy(pTile, pPreview, imgLen);
                    }

                    break;
                }
                if (i < 0) continue;
                break;
            case 100:   // replace color
                if (pColorPickA->value < 0) sprintf(buffer, "Tile color is not selected.");
                else if (pColorPickB->value < 0) sprintf(buffer, "Palette color is not selected.");
                else if (pColorPickA->value == pColorPickB->value) sprintf(buffer, "Colors are same.");
                if (!buffer[0])
                {
                    if ((j = artedReplaceColor(nPreview, (BYTE)pColorPickA->value, (BYTE)pColorPickB->value)) > 0)
                    {
                        tileColors[pColorPickA->value] = 0;  // disable this color
                        tileColors[pColorPickB->value] = 3;  // enable this color and mark as used
                        memcpy(&pal[pColorPickA->value], &pal[pColorPickB->value], sizeof(RGB));
                    }

                    sprintf(buffer, "%d pixels replaced.", j);
                    strupr(buffer);

                }
                else
                {
                    Alert(buffer); buffer[0] = '\0';
                    if (pColorPickA->value >= 0 && tileColors[pColorPickA->value] != 3)
                        tileColors[pColorPickA->value] = 2;
                }
                memset(paletteColors, 1, sizeof(paletteColors));
                if (pColorPickB->value >= 0) paletteColors[pColorPickB->value] = 2; // make this color selected by default
                continue;
            case 101:   // reset changes
                if (memcmp(pTile, pPreview, imgLen) == 0) continue;
                else if (!Confirm("Reset all tile colors?")) continue;
                memcpy(tileColors, bTileColors, sizeof(bTileColors));
                memcpy(pPreview, pTile, imgLen);
                memcpy(pal, gamepal, sizeof(pal));
                continue;
            case 102: // paint with palette
                if (memcmp(pTile, pPreview, imgLen) != 0)
                {
                    if (!Confirm("Keep current colors?"))
                    {
                        memcpy(tileColors, bTileColors, sizeof(bTileColors));
                        memcpy(pPreview, pTile, imgLen);
                        memcpy(pal, gamepal, sizeof(pal));
                    }
                }

                if (artedDlgPaint(nPreview) <= 0)
                    continue;

                memset(tileColors, 0, sizeof(tileColors));
                for (i = 0; i < imgLen; tileColors[pPreview[i++]] = 1);
                if (palLoad(gPaths.palPaint, tmp) < 0) continue;
                memcpy(pal, tmp, sizeof(pal));
                continue;
        }

        break;
    }

    tilePurgeTile(nPreview, TRUE);
    //return (BOOL)(memcmp(pTile, pPreview, imgLen) != 0);
    return FALSE;
}

void artedCopyTile(int nTileSrc, int nTileDest, int what) {

    if (nTileSrc == nTileDest)
        return;

    if (!what || what >= kAll)
        what = (kImage | kAnimation | kShade | kSurface | kVoxelID | kView | kOffsets);

    tileLoadTile(nTileSrc);

    if (what & kImage)
    {
        if (tilesizx[nTileDest] || tilesizy[nTileDest]) tileFreeTile(nTileDest);
        tileAllocTile(nTileDest, tilesizx[nTileSrc], tilesizy[nTileSrc], 0, 0);
        memcpy((void*)waloff[nTileDest], (void*)waloff[nTileSrc], tilesizx[nTileSrc]*tilesizy[nTileSrc]);
    }

    if (what & kOffsets)
    {
        panm[nTileDest].xcenter = panm[nTileSrc].xcenter;
        panm[nTileDest].ycenter = panm[nTileSrc].ycenter;
    }

    if (what & kView)
    {
        viewType[nTileDest] = viewType[nTileSrc];
        if (!isExternalModel(nTileDest))
            panm[nTileDest].view = viewType[nTileDest];
    }

    if (what & kAnimation)
    {
        panm[nTileDest].type    = panm[nTileSrc].type;
        panm[nTileDest].speed   = panm[nTileSrc].speed;
        panm[nTileDest].frames  = panm[nTileSrc].frames;
    }

    if (what & kShade)
        tileShade[nTileDest] = tileShade[nTileSrc];

    if (what & kSurface)
        surfType[nTileDest] = surfType[nTileSrc];

    if (what & kVoxelID)
        voxelIndex[nTileDest] = voxelIndex[nTileSrc];
}

int artedDlgCopyTiles(int nTile, int total) {

    BOOL asked = FALSE;
    int i, j, hdx, cur, tile, err = 1;

    for (i = 0; i < gMaxTiles; i++)
    {
        if (tileInHglt(i) && (!tilesizx[i] || !tilesizy[i]))
            total--;
    }

    if (total <= 0) { Alert("No selection or all tiles are empty!"); return 0; }
    else if (total > 1 && !Confirm("Copy %d tiles?", total))
        return -1;

    // check distance between tiles
    for (j = 0; j < gMaxTiles && !tileInHglt(j); j++);
    for (cur = 0; cur < total; cur++)
    {
        if (tileInHglt(j++)) continue;
        else if ((i = YesNoCancel("Keep distance in tact?")) != mrCancel) break;
        else return -1;
    }

    while ( 1 )
    {
        tile = nTile;
        for (hdx = 0, cur = 0; hdx < gMaxTiles && cur < total; hdx++)
        {
            while(tile < gMaxTiles && isSysTile(tile))
                tile++;

            if (tile >= gMaxTiles)
            {
                err = -1;
                break;
            }
            else if (!tileInHglt(hdx))
            {
                if (i == mrOk && cur > 0) tile++;
                continue;
            }
            else if (!asked && (tilesizx[tile] || tilesizy[tile]))
            {
                asked = TRUE;
                if (!Confirm("Overwrite destination tile(s)?"))
                {
                    err = 0;
                    break;
                }
            }

            if (!err)
            {
                artedCopyTile(hdx, tile);
                if (artedGetFile(tile) == NULL)
                    ThrowError("No file for tile #%d!", tile);

                artedArtDirty(tile, kDirtyAll);
                hgltTileRem(hdx);
            }

            cur++, tile++;
        }

        if (!err) return cur;
        else if (err > 0)
        {
            err = 0;
            continue;
        }
        else if (err < 0)
        {
            Alert("Error #%d: %s.", abs(err), retnCodeCheck(err, gCopyErrors));
            break;
        }

    }


    return 0;
}

int artedBatchProc(HTILEFUNC1 tileFunc, int data, char dirty) {

    int cnt = hgltTileCount();
    if (cnt <= 1 || !tileInHglt(gArtEd.nTile))
    {

        tileFunc(gArtEd.nTile, data);
        if (dirty)
            artedArtDirty(gArtEd.nTile, dirty);

        return 1;
    }

    return hgltTileCallFunc(tileFunc, data, dirty);

}

void hgltTileSelectRange(int nTile) {

    int i, j, total = hgltTileCount();

    if (total <= 0 || (total < 2 && tileInHglt(nTile)))
    {
        for (i = 0; i <= nTile; gHgltTiles[i++].ok = 1);
        return;
    }

    for (j = nTile; j >= 0; j--)
    {
        if (j == nTile || !tileInHglt(j)) continue;
        for (i = j; i <= nTile; gHgltTiles[i++].ok = 1);
        return;
    }

    for (j = nTile; j < gMaxTiles; j++)
    {
        if (j == nTile || !tileInHglt(j)) continue;
        for (i = nTile; i < j; gHgltTiles[i++].ok = 1);
        return;
    }
}

int hgltTileCount() {

    int i, cnt = 0;
    for (i = 0; i < kMaxTiles; i++)
    {
        if (tileInHglt(i))
            cnt++;
    }

    return cnt;
}

int hgltTileCallFunc(HTILEFUNC1 tileFunc, int data, char dirty) {

    int i, cnt = 0;
    for (i = kMaxTiles - 1; i >= 0; i--)
    {
        if (tileInHglt(i) && !isSysTile(i))
        {

            cnt++;
            tileFunc(i, data);
            if (dirty)
                artedArtDirty(i, dirty);

        }
    }



    return cnt;
}

void hgltTileReset() { for (int i = 0; i < kMaxTiles; i++) gHgltTiles[i].ok = 0; }
void hgltTileAdd(int nTile) { gHgltTiles[nTile].ok = 1; }
void hgltTileRem(int nTile) { gHgltTiles[nTile].ok = 0; }
void hgltTileToggle(int nTile) { gHgltTiles[nTile].ok ^=1; }

void artedSaveChanges() {

    plsWait();
    artedCleanUp();

    extVoxUninit();
    tileUninitSystemTiles();
    artedSaveArtFiles();
    artedSaveDatFiles();
    tilePurgeAll(TRUE);
    tileInitFromIni();
    tileInitSystemTiles();
    extVoxInit();

    gArtEd.asksave = kDirtyNone;
    memset(gDirtTiles, 0, sizeof(gDirtTiles));
}

void artedRevertChanges() {

    plsWait();

    extVoxUninit();
    tileUninitSystemTiles();
    tilePurgeAll(TRUE);
    tileInitFromIni();
    tileInitSystemTiles();
    extVoxInit();

    gArtEd.asksave = kDirtyNone;
    memset(gDirtTiles, 0, sizeof(gDirtTiles));

}

int artedDlgSelectTilePart(char* title) {

    int retn = 0;
    if (createCheckboxList(revertMenu, LENGTH(revertMenu), title, TRUE) != mrCancel)
    {
        if (revertMenu[0].option) retn |= (kImage | kOffsets);
        if (revertMenu[1].option) retn |= (kAnimation | kView);
        if (revertMenu[2].option) retn |= (kSurface | kShade | kVoxelID);
    }

    return retn;
}


void artedRevertTile(int nTile, int what) {

    int32_t tofs = 0;
    int i;  short tsx = 0, tsy = 0;
    BOOL alloc = (tilefileoffs[nTile] == 0);
    PICANM pnm; memset(&pnm, 0, sizeof(pnm));

    if (!alloc && (tilefilenum[nTile] == 255 || !fileExists(gArtFiles[tilefilenum[nTile]].path))) return;
    else if (!what || what >= kAll)
        what = (kImage | kAnimation | kShade | kSurface | kVoxelID | kView | kOffsets);

    if (alloc || (i = readTileArt(nTile, gArtFiles[tilefilenum[nTile]].path, &tsx, &tsy, NULL, &tofs, &pnm)) >= 0)
    {
        if (what & kImage)
        {
            tileFreeTile(nTile);
            tilesizx[nTile] = tsx;
            tilesizy[nTile] = tsy;
            CalcPicsiz(nTile, tilesizx[nTile], tilesizy[nTile]);
            tilefileoffs[nTile] = tofs;
        }

        if (what & kAnimation)
        {
            panm[nTile].frames      = pnm.frames;
            panm[nTile].type        = pnm.type;
            panm[nTile].speed       = pnm.speed;
            panm[nTile].update      = pnm.update;
        }

        if (what & kOffsets)
        {
            panm[nTile].xcenter         = pnm.xcenter;
            panm[nTile].ycenter         = pnm.ycenter;
        }

        if (what & kView)
        {
            viewType[nTile] = (BYTE)pnm.view;
            if (!isExternalModel(nTile))
                panm[nTile].view = viewType[nTile];
        }

        if (what & kSurface) readTileSurface(nTile, gPaths.surfDAT, &surfType[nTile]);
        if (what & kVoxelID) readTileVoxel(nTile, gPaths.voxelDAT, &voxelIndex[nTile]);
        if (what & kShade)   readTileShade(nTile, gPaths.shadeDAT, &tileShade[nTile]);
    }

    return;
}

void artedCleanUpTile(int nTile) {

    BOOL animclr = FALSE;
    PICANM* pnm = &panm[nTile];
    switch(viewType[nTile]) {
        case kSprViewVox:
        case kSprViewVoxSpin:
            if (voxelIndex[nTile] >= 0)
            {
                if (voxelIndex[nTile] >= kMaxVoxels || !gSysRes.Lookup(voxelIndex[nTile], "KVX"))
                    voxelIndex[nTile] = -1;
            }
            if (voxelIndex[nTile] < 0)
            {
                viewType[nTile] = kSprViewSingle;
                if (!isExternalModel(nTile))
                    pnm->view = viewType[nTile];
            }
            break;
    }

    if (!animclr && pnm->type)      animclr = (!pnm->frames || !pnm->speed);
    if (!animclr && pnm->frames)    animclr = (!pnm->type   || !pnm->speed);
    if (!animclr && pnm->speed)     animclr = (!pnm->type   || !pnm->frames);
    if (animclr)
    {
        pnm->type       = 0;
        pnm->frames     = 0;
        pnm->speed      = 0;
    }

    if (!tilesizx[nTile]) tilesizy[nTile] = 0;
    if (!tilesizy[nTile]) tilesizx[nTile] = 0;
    if (!tilesizx[nTile])
    {
        tileShade[nTile]    = 0;
        surfType[nTile]     = 0;
    }
}

void artedCleanUp() {

    for (int i = 0; i < gMaxTiles; artedCleanUpTile(i++));

}

void artedViewZoomReset() {

    toolGetResTableValues();

}


int readArtHead(int hFile, int* tstart, int* tend, int* sixofs, int* siyofs, int* pnmofs, int* datofs) {

    int i = 0, slen, flen;
    char tmp[256];

    dassert(hFile >= 0);
    if ((flen = lseek(hFile, 0, SEEK_END)) <= sizeof(int32_t)<<2)
        return -1;

    lseek(hFile, 0, SEEK_SET);
    read(hFile, &i, sizeof(int32_t));

    // check standard header by reading artversion
    if (i != 0x0001)
    {
        // check for signs in the file
        for (i = 0; i < LENGTH(gArtSigns); i++)
        {
            NAMED_TYPE* offset = &gArtSigns[i];
            lseek(hFile, 0, SEEK_SET); memset(tmp, 0, sizeof(tmp));
            if ((slen = strlen(offset->name)) >= sizeof(tmp)) continue;
            else if (read(hFile, tmp, slen) != slen) return -1;
            else if (strcmp(tmp, offset->name) != 0) continue;
            switch (offset->id) {
                case kArtSignBUILDART:
                    read(hFile, &i, sizeof(int32_t));           // artversion check
                    if (i != 0x0001) return -1;                 // it must be 1
                    lseek(hFile, sizeof(int32_t), SEEK_CUR);    // skip numtiles
                    break;
                case kArtSignBAFED:
                    lseek(hFile, 0, SEEK_SET);
                    lseek(hFile, sizeof(int32_t)<<1, SEEK_CUR); // skip artversion, numtiles
                    break;
            }

            break;
        }

        // unknown art file format
        if (i >= LENGTH(gArtSigns))
            return -2;
    }
    else
    {
        lseek(hFile, sizeof(int32_t), SEEK_CUR); // only skip numtiles
    }

    if (read(hFile, tstart, sizeof(int32_t)) != sizeof(int32_t)) return -1;
    if (read(hFile, tend,   sizeof(int32_t)) != sizeof(int32_t)) return -1;

    i = *tend - *tstart + 1;
    *sixofs = tell(hFile);
    *siyofs = *sixofs + (sizeof(int16_t)*i);
    *pnmofs = *siyofs + (sizeof(int16_t)*i);
    *datofs = *pnmofs + (sizeof(PICANM)*i);

    return (*sixofs >= flen || *siyofs >= flen || *pnmofs >= flen || *datofs > flen) ? -1 : 0;
}


int readArtHead(char* file, int* tstart, int* tend, int* sixofs, int* siyofs, int* pnmofs, int* datofs) {

    int hFile, retn = -3;
    if ((hFile = open(file, O_BINARY|O_RDONLY, S_IWRITE)) >= 0)
    {
        retn = readArtHead(hFile, tstart, tend, sixofs, siyofs, pnmofs, datofs);
        close(hFile);
    }

    return retn;
}

int readTileArt(int nTile, int hFile, short* six, short* siy, BYTE** image, int32_t* imgofs, PICANM* pnm)
{
    short tsx = -1, tsy = -1;
    int start, end, sixofs, siyofs, pnmofs, datofs, imglen;
    if ((readArtHead(hFile, &start, &end, &sixofs, &siyofs, &pnmofs, &datofs) != 0)) return -1;
    else if ((end - start + 1) <= 0 || nTile < start || nTile > end)
        return -2;

    // must go through resolution data
    // to determine image data offset
    while ( 1 )
    {
        lseek(hFile, sixofs, SEEK_SET); sixofs+=read(hFile, &tsx, sizeof(int16_t));
        lseek(hFile, siyofs, SEEK_SET); siyofs+=read(hFile, &tsy, sizeof(int16_t));
        if (start == nTile)
            break;

        pnmofs+=sizeof(PICANM), datofs+=(tsx*tsy);
        start++;
    }

    if (lseek(hFile, datofs, SEEK_SET))
    {
        if (image && (imglen = tsx*tsy) > 0)
        {
            dassert(six != NULL && siy != NULL);
            if (*image)
            {
                dassert(*image == (BYTE*)waloff[nTile]);
                dassert(waloff[nTile] != NULL);
            }
            else
            {
                *image = (BYTE*)Resource::Alloc(imglen);
            }

            read(hFile, *image, imglen);
        }

        if (imgofs) *imgofs = datofs;
        if (six)    *six = tsx;
        if (siy)    *siy = tsy;

        if (pnm)
        {
            lseek(hFile, pnmofs, SEEK_SET);
            read(hFile, pnm, sizeof(PICANM));
        }

        return 0;
    }

    return -4;
}

int readTileArt(int nTile, char* file, short* six, short* siy, BYTE** image, int32_t* imgofs, PICANM* pnm) {

    int i, hFile;
    if ((hFile = open(file, O_BINARY|O_RDONLY, S_IREAD|S_IWRITE)) < 0)
        return -1;

    i = readTileArt(nTile, hFile, six, siy, image, imgofs, pnm);
    close(hFile);
    return i;
}


int readTileShade(int nTile, char* file, schar* out) {

    int hFile, seek, leng; *out = 0;
    if ((hFile = open(file, O_BINARY|O_RDONLY, S_IWRITE)) < 0)
        return -1;

    leng = filelength(hFile);
    if ((seek = lseek(hFile, sizeof(schar)*nTile, SEEK_SET)) >= 0 && seek < leng)
    {
        read(hFile, out, sizeof(schar));
        close(hFile);
        return 0;
    }

    close(hFile);
    return -2;
}

int readTileSurface(int nTile, char* file, BYTE* out) {

    int hFile, seek, leng; *out = 0;
    if ((hFile = open(file, O_BINARY|O_RDONLY, S_IWRITE)) < 0)
        return -1;

    leng = filelength(hFile);
    if ((seek = lseek(hFile, sizeof(BYTE)*nTile, SEEK_SET)) >= 0 && seek < leng)
    {
        read(hFile, out, sizeof(BYTE));
        close(hFile);
        return 0;
    }

    close(hFile);
    return -2;
}

int readTileVoxel(int nTile, char* file, short* out) {

    int hFile, seek, leng; *out = -1;
    if ((hFile = open(file, O_BINARY|O_RDONLY, S_IWRITE)) < 0)
        return -1;

    leng = filelength(hFile);
    if ((seek = lseek(hFile, sizeof(short)*nTile, SEEK_SET)) >= 0 && seek < leng)
    {
        read(hFile, out, sizeof(short));
        close(hFile);
        return 0;
    }

    close(hFile);
    return -2;
}

BOOL xartOffsets(int hFile, int* blocksCount, int* mapofs) {

    char tmp[32]; memset(tmp, 0, sizeof(tmp));
    int sigLen = strlen(kArtBlockMapSign)+1; tmp[sigLen] = '\0';

    *blocksCount = 0, *mapofs = 0;
    lseek(hFile, -sigLen, SEEK_END); read(hFile, tmp, sigLen);
    if (tmp[0] > 0 && strcmp(&tmp[1], kArtBlockMapSign) == 0)
    {
        *blocksCount    = tmp[0];
        *mapofs         = lseek(hFile, -((tmp[0]*sizeof(int))+sigLen), SEEK_CUR);

        return TRUE;
    }

    return FALSE;

}

BOOL canEdit() {
    if (gArtEd.mode <= kArtEdModeNone)
    {
        scrSetMessage("You must enable edit mode first.");
        BeepFail();
        return FALSE;
    }

    if (isSysTile(gArtEd.nTile))
    {
        scrSetMessage("This tile is system reserved.");
        BeepFail();
        return FALSE;
    }

    return TRUE;
}

void artedLogMsg(char *__format, ...) {

    int i; char tmp[512];

    va_list argptr;
    va_start(argptr, __format);
    vsprintf(tmp, __format, argptr);
    va_end(argptr);

    buildprintf("XMPARTED: %s", tmp);
}

// compares IN palette RGBs against game palette RGBs
/* BOOL isPalookup(PALETTE in, int allowRng) {

    int i, j, k;
    for (i = 0; i < 256; i++)
    {
        for (j = 0; j < 256; j++)
        {
            if (memcmp(&in[i], &palette[j], sizeof(RGB)) == 0) break;
        }

        if (j >= 256) {
            Alert("%d", i);
            return FALSE;
        }
    }

    return TRUE;
} */


/* void tileEdAnimSpeedSet(int nTile, int value) { panm[nTile].speed = value; }
void tileEdAnimFramesSet(int nTile, int value) { panm[nTile].frames = value; } */