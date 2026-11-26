/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2025: Adapted for XMAPEDIT by NoOne.
// Map editing key input processing for shared, 2D
// and 3D modes.
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

#include "xmpstub.h"
#include "maproc.h"
#include "xmpsky.h"
#include "grdshd.h"
#include "screen.h"
#include "aadjust.h"
#include "preview.h"
#include "mapcmt.h"
#include "xmpview.h"
#include "tracker.h"
#include "tile.h"
#include "xmpexplo.h"

static char* batchFormatStr = "Do same job for %d other %ss in a highlight?"; 
char gBatchMode = 0x01;
TIMER gObjectLock;


#define IS_FLOOR (searchstat == OBJ_FLOOR)
#define IS_WALL  (searchstat == OBJ_WALL)
#define IS_EMPTY (searchstat == OBJ_NONE)

const char* gTranslucLevNames[3] =
{
    "not",
    "less",
    "mostly",
};

char* gSectorDialogNames[2] =
{
    "Main properties",
    "FX properties",
};

int dlgZHeightOff(char *title, int z1, int z2, char bottom, char dir)
{
    const int l = gfxGetTextLen(title, pFont)+8;
    const int wh = ClipRange(l, 154, xdim-4);
    const int hwh = wh >> 1;
    static char sel = 1;
    int nVal;

    nVal = (z1 - z2) / 256;

    Window dialog(0, 0, wh+14, 62, title);
    EditNumber *en = new EditNumber(4, 4, wh, 18, nVal);
    TextButton *bHeigh = new TextButton(4, 22, hwh, 20, "Height off", 100);
    TextButton *bShift = new TextButton(hwh+4, 22, hwh, 20, "Shift to", 101);

    (sel) ? bHeigh->fontColor = kColorBlue : bShift->fontColor = kColorBlue;

    dialog.Insert(en);
    dialog.Insert(bHeigh);
    dialog.Insert(bShift);

    if (ShowModal(&dialog) != mrCancel)
    {
        Widget* pFocus = ((Container*)dialog.focus)->focus;
        sel = ((pFocus == bHeigh) || (sel && pFocus == en));
        nVal = en->value * 256;

        if (sel)
        {
            nVal = (bottom) ? (z2 + nVal) - z1 : (z1 - nVal) - z2;
        }
        else
        {
            nVal = (dir) ? nVal : -nVal;
        }

        return nVal;
    }

    return 0;
}

static int lightBombLoadPreset(IniFile* pIni, LIGHT_BOMB* pPrefs, char check = 0)
{
    char *pGroup = "LightBomb";

    if (check && !pIni->SectionExists(pGroup))
    {
        Alert("\"%s\" is not Light Bomb Preset file!");
        return 0;
    }

    pPrefs->intensity   = ClipLow(pIni->GetKeyInt(pGroup,   "Intensity",      8),        0);
    pPrefs->attenuation = ClipLow(pIni->GetKeyInt(pGroup,   "Attenuation",    8192),     0);
    pPrefs->reflections = ClipRange(pIni->GetKeyInt(pGroup, "Reflections",    4),        0, 256);
    pPrefs->rampDist    = ClipLow(pIni->GetKeyInt(pGroup,   "RampDist",       65536),    0);
    pPrefs->maxBright   = ClipRange(pIni->GetKeyInt(pGroup, "MaxBright",      0),      -128, 127);

    return 1;
}

static int dlgLightBombOptions(LIGHT_BOMB* pPrefs)
{
    char filepath[BMAX_PATH], *pPath, *pGroup = "LightBomb";
    Widget* pFocus; LIGHT_BOMB buf;
    int nResult;

    memcpy(&buf, pPrefs, sizeof(buf));

    while( 1 )
    {
        Window dialog(0, 0, 244, 180, "Light Bomb Setup");

        Label *lIntensity           = new Label(8, 10, "Intensity");
        EditNumber *eIntensity      = new EditNumber(110, 6, 44, 18, buf.intensity, '\0', 0, 65536);

        Label *lReflections         = new Label(8, 30, "Reflections");
        EditNumber *eReflections    = new EditNumber(110, 26, 44, 18, buf.reflections, '\0', 0, 256);

        Label *lAttenuation         = new Label(8, 50, "Attenuation");
        EditNumber *eAttenuation    = new EditNumber(110, 46, 44, 18, buf.attenuation, '\0', 0, 65536);

        Label *lBright              = new Label(8, 70, "Max brightness");
        EditNumber *eBright         = new EditNumber(110, 66, 44, 18, buf.maxBright, '\0', -128, 127);

        Label *lDist                = new Label(8, 90, "Ramp distance");
        EditNumber *eDist           = new EditNumber(110, 86, 44, 18, buf.rampDist, '\0', 0);

        FieldSet* fOpts             = new FieldSet(8, 8, 222, 108, "OPTIONS", kColorRed, kColorGrey25);
        FieldSet* fPresets          = new FieldSet(160, 6, 56, 96, NULL, kColorRed, kColorGrey25);

        TextButton *bLoad           = new TextButton(4, 4, 48, 30, "Load", mrLoad);
        TextButton *bSave           = new TextButton(4, 34, 48, 30, "Save", mrSaveAs);
        TextButton *bDefault        = new TextButton(4, 64, 48, 30, "Reset", mrReload);

        TextButton *bOk             = new TextButton(8,  130, 60, 24, "Ok", mrOk);
        TextButton *bCancel         = new TextButton(70, 130, 60, 24, "Cancel", mrCancel);
        TextButton *bHelp           = new TextButton(208, 130, 24, 24, "?", mrHelp);

        fPresets->Insert(bLoad);
        fPresets->Insert(bSave);
        fPresets->Insert(bDefault);

        fOpts->Insert(lIntensity);
        fOpts->Insert(eIntensity);

        fOpts->Insert(lReflections);
        fOpts->Insert(eReflections);

        fOpts->Insert(lAttenuation);
        fOpts->Insert(eAttenuation);

        fOpts->Insert(lBright);
        fOpts->Insert(eBright);

        fOpts->Insert(lDist);
        fOpts->Insert(eDist);

        fOpts->Insert(fPresets);

        dialog.Insert(fOpts);
        dialog.Insert(bOk);
        dialog.Insert(bCancel);
        dialog.Insert(bHelp);

        bOk->fontColor = kColorBlue;
        bCancel->fontColor = kColorRed;
        bHelp->fontColor = kColorMagenta;

        if ((nResult = ShowModal(&dialog)) == mrCancel)
            break;

        if (nResult == mrOk)
        {
            if (fPresets->focus != &fPresets->head) pFocus = ((Container*)fPresets->focus)->focus;
            else if (dialog.focus != &dialog.head)  pFocus = ((Container*)dialog.focus)->focus;
            else pFocus = NULL;

            if (pFocus == bDefault)     nResult = bDefault->result;
            else if (pFocus == bSave)   nResult = bSave->result;
            else if (pFocus == bLoad)   nResult = bLoad->result;
            else if (pFocus == bHelp)   nResult = bHelp->result;
            else
            {
                pPrefs->intensity   = eIntensity->value;
                pPrefs->reflections = eReflections->value;
                pPrefs->attenuation = eAttenuation->value;
                pPrefs->rampDist    = eDist->value;
                pPrefs->maxBright   = eBright->value;
                break;
            }
        }

        if (nResult == mrHelp)
        {
            sprintf(filepath, "%s/%s", kXmpLightBombDir, "ltbomb.txt");
            ShowFileContents(filepath);
            continue;
        }

        if (nResult == mrReload)
        {
            if (Confirm("Reset to defaults?"))
            {
                sprintf(filepath, "%s/%s", kXmpLightBombDir, "default.lbp");
                IniFile ini(filepath);

                lightBombLoadPreset(&ini, &buf);
            }

            continue;
        }

        if (nResult == mrSaveAs)
        {
            if ((pPath = browseSave(gPaths.lbps, ".lbp")) != NULL)
            {
                IniFile ini(pPath);
                ini.PutKeyInt(pGroup, "Intensity",     eIntensity->value);
                ini.PutKeyInt(pGroup, "Reflections",   eReflections->value);
                ini.PutKeyInt(pGroup, "Attenuation",   eAttenuation->value);
                ini.PutKeyInt(pGroup, "MaxBright",     eBright->value);
                ini.PutKeyInt(pGroup, "RampDist",      eDist->value);

                if (ini.Save())
                    Alert("Preset has been saved!");
            }

            continue;
        }

        if (nResult == mrLoad)
        {
            if ((pPath = browseOpen(gPaths.lbps, ".lbp")) != NULL)
            {
                IniFile ini(pPath);
                lightBombLoadPreset(&ini, &buf, 1);
            }

            continue;
        }
    }

    return (nResult == mrOk);
}

int dlgClipdist(char *title, int nVal, int nValAuto, int nDefault)
{
    const int l = gfxGetTextLen(title, pFont)+8;
    const int wh = ClipRange(l, 154, xdim-4);
    const int hwh = wh >> 1;

    char buf[16];
    sprintf(buf, "Auto [%d]", nValAuto);

    Window dialog(0, 0, wh+14, 62, title);
    EditNumber *en = new EditNumber(4, 4, wh, 18, nVal);
    TextButton *bCur = new TextButton(4, 22, hwh, 20, "Ok", 100);
    TextButton *bAuto = new TextButton(hwh+4, 22, hwh, 20, buf, 101);

    dialog.Insert(en);
    dialog.Insert(bCur);
    dialog.Insert(bAuto);

    if (ShowModal(&dialog) != mrCancel)
    {
        Widget* pFocus = ((Container*)dialog.focus)->focus;
        nVal = (pFocus == en || pFocus == bCur) ? en->value : nValAuto;
        return ClipRange(nVal, 0, 255);
    }

    return nDefault;
}

char helperTranslateToSector(char allowSky = 1)
{
    switch(searchstat)
    {
        case OBJ_SPRITE:
        case OBJ_WALL:
            searchsector = sectorhighlight;
            // no break
        case OBJ_FLOOR:
        case OBJ_CEILING:
            switch(gScreen2D.prefs.showMap)
            {
                case 1: searchstat = OBJ_FLOOR;     return 1;
                case 2: searchstat = OBJ_CEILING;   return 2;
            }
            break;
    }

    scrSetMessage("Must enable textured map first");
    return 0;
}

////////////////////////////////////////////////////
/***********************************************/
/** SHARED MAP EDITING INPUT FUNCTIONS        **/
/***********************************************/
//////////////////

static char edKeyProcShared_KEY_1(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; walltype* pWall;
    char* msg = buffer;
    int i;

    switch (searchstat)
    {
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            ProcessSprites((pSpr->cstat & kSprOneSided) ? sprCstatRem : sprCstatAdd, kSprOneSided, BMODE, msg);
            msg += sprintf(msg, "%s %s one-sided", buffer, isNot(pSpr->cstat & kSprOneSided));

            if ((pSpr->cstat & kSprRelMask) >= kSprFloor && (pSpr->cstat & kSprOneSided))
            {
                ProcessSprites(spr_Floor_OneSide_HandleFlip, posz, BMODE, NULL);
                msg+=sprintf(msg, " (visible from %s)", (pSpr->cstat & kSprFlipY) ? "below" : "above");
            }
            scrSetMessage(buffer);
            return PROC_OKUB;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            if (ED2D) break;
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            pWall = &wall[searchwall];
            if (ProcessWalls((pWall->cstat & kWallOneWay) ? wallCstatRem : wallCstatAdd_OneWay, kWallOneWay, false, BMODE, buffer))
            {
                scrSetMessage("%s %s one-way", buffer, isNot(pWall->cstat & kWallOneWay));
                return PROC_OKUB;
            }
            break;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_ENTER(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect = NULL; walltype* pWall = NULL; spritetype* pSpr = NULL;
    int nMark0, nMark1, oCstat, i;
    char ok = 0;

    if (ED3D && gPreviewMode)
    {
        INPUTPROC* pInput = &gEditInput3D[KEY_CAPSLOCK];
        if (pInput->pFunc)
            return pInput->pFunc(key, ctrl, shift, alt);

        return PROC_FAILB;
    }

    if (IS_EMPTY)
        return PROC_FAILB;

    if (somethingintab == OBJ_NONE)
    {
        scrSetMessage("There is nothing to paste.");
        return PROC_FAILB;
    }

    if ((ctrl && alt) || ED2D)
    {
        if (somethingintab != searchstat)
        {
            if (somethingintab != OBJ_SECTOR)
            {
                scrSetMessage("Can't copy from %s to %s", gSearchStatNames[somethingintab], GetHoverName());
                return PROC_FAILB;
            }

            if ((searchsector = getSector()) < 0)
            {
                scrSetMessage("Failed to get destination sector.");
                return PROC_FAILB;
            }

            searchstat = OBJ_SECTOR;
        }
    }

    if (ctrl && alt)
    {
        // call the advanced clipboard

        if (tempidx < 0)
            return PROC_FAILB;

        i = -1;
        switch(searchstat)
        {
            case OBJ_WALL:
            case OBJ_MASKED:
                i = dlgCopyObjectSettings(dlgXWall, OBJ_WALL, tempidx, searchwall, wallInHglt(searchwall));
                break;
            case OBJ_SPRITE:
                i = dlgCopyObjectSettings(dlgXSprite, OBJ_SPRITE, tempidx, searchwall, sprInHglt(searchwall));
                break;
            case OBJ_SECTOR:
            case OBJ_FLOOR:
            case OBJ_CEILING:
                DIALOG_ITEM* pDlg = dlgXSector;
                if (sector[tempidx].extra > 0 && testXSectorForLighting(sector[tempidx].extra))
                {
                    switch(showButtons(gSectorDialogNames, LENGTH(gSectorDialogNames), "Show") - mrUser)
                    {
                        case 1:     pDlg = dlgXSectorFX;    break;
                        default:    pDlg = NULL;            break;
                    }
                }
                
                if (pDlg)
                    i = dlgCopyObjectSettings(pDlg, OBJ_SECTOR, tempidx, searchsector, sectInHglt(searchsector));
                
                break;
        }

        if (i >= 0)
        {
            scrSetMessage("Selected properties were copied for %d objects", i);
            return PROC_OKUB;
        }

        return PROC_FAILB;
    }

    if (ED3D)
    {
        if (ctrl)
        {
            // paste to all wall loop
            if (IsHoverWall())
            {
                i = searchwall;
                do
                {
                    pWall = &wall[i];

                    if (shift)
                    {
                        pWall->shade = tempshade;
                        pWall->pal = temppal;
                    }
                    else
                    {
                        pWall->picnum = temppicnum;
                        switch (somethingintab)
                        {
                            case OBJ_WALL:
                            case OBJ_MASKED:
                                pWall->xrepeat  = tempxrepeat;
                                pWall->yrepeat  = tempyrepeat;
                                pWall->cstat    = tempcstat;
                                break;
                        }
                        fixrepeats(i);
                    }

                    i = pWall->point2;
                }
                while (i != searchwall);
                return PROC_OKUB;
            }

            // paste to all parallax ceilings or floors
            if (IsHoverSector())
            {
                i = numsectors;
                while(--i >= 0)
                {
                    if (!isSkySector(i, searchstat))
                        continue;

                    pSect = &sector[i];

                    if (IS_FLOOR)
                    {
                        pSect->floorpicnum      = temppicnum;
                        pSect->floorshade       = tempshade;
                        pSect->floorpal         = temppal;
                    }
                    else
                    {
                        pSect->ceilingpicnum    = temppicnum;
                        pSect->ceilingshade     = tempshade;
                        pSect->ceilingpal       = temppal;
                    }

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            if (IS_FLOOR)
                            {
                                pSect->floorxpanning = tempxrepeat;
                                pSect->floorypanning = tempyrepeat;
                                pSect->floorstat = (BYTE)(tempcstat | kSectParallax);
                            }
                            else
                            {
                                pSect->ceilingxpanning = tempxrepeat;
                                pSect->ceilingypanning = tempyrepeat;
                                pSect->ceilingstat = (BYTE)(tempcstat | kSectParallax);
                            }
                            break;
                    }
                }

                return (pSect) ? PROC_OKUB : PROC_FAILB;
            }

            return PROC_FAILB;
        }

        if (shift)  // paste shade and palette
        {
            if (IsHoverWall())
            {
                wall[searchwall].shade  = tempshade;
                wall[searchwall].pal    = temppal;
                return PROC_OKUB;
            }

            if (IsHoverSector())
            {
                if (sectInHglt(searchsector))
                {
                    i = highlightsectorcnt;
                    while(--i >= 0)
                    {
                        pSect = &sector[highlightsector[i]];
                        pSect->visibility = cpysector.visibility;

                        if (IS_FLOOR)
                        {
                            pSect->floorshade   = tempshade;
                            pSect->floorpal = temppal;
                        }
                        else
                        {
                            pSect->ceilingshade = tempshade;
                            pSect->ceilingpal   = temppal;
                        }
                    }
                }
                else
                {
                    pSect = &sector[searchsector];
                    pSect->visibility = cpysector.visibility;

                    if (IS_FLOOR)
                    {
                        pSect->floorshade   = tempshade;
                        pSect->floorpal     = temppal;
                    }
                    else
                    {
                        pSect->ceilingshade = tempshade;
                        pSect->ceilingpal   = temppal;
                    }

                    // if this sector is a parallaxed sky...
                    if (isSkySector(searchsector, searchstat))
                    {
                        // propagate shade data on all parallaxed sky sectors
                        i = numsectors;
                        while(--i >= 0)
                        {
                            if (!isSkySector(i, searchstat))
                                continue;

                            pSect = &sector[i];
                            pSect->visibility = cpysector.visibility;

                            if (IS_FLOOR)
                            {
                                pSect->floorshade   = tempshade;
                                pSect->floorpal     = temppal;
                            }
                            else
                            {
                                pSect->ceilingshade = tempshade;
                                pSect->ceilingpal   = temppal;
                            }
                        }
                    }
                }

                return PROC_OKUB;
            }

            if (IsHoverSprite())
            {
                pSpr = &sprite[searchwall];
                pSpr->shade = tempshade;
                pSpr->pal = temppal;

                return PROC_OKUB;
            }

            return PROC_FAILB;
        }

        if (!alt)
        {
            if (IsHoverWall())
            {
                pWall = &wall[searchwall];

                pWall->pal = temppal;
                pWall->shade = tempshade;

                if (searchstat != OBJ_WALL)
                {
                    pWall->overpicnum = temppicnum;
                    if (pWall->nextwall >= 0)
                        wall[pWall->nextwall].overpicnum = temppicnum;
                }
                else
                {
                    pWall->picnum = temppicnum;
                }

                if (somethingintab == searchstat)
                {
                    oCstat          = pWall->cstat;
                    pWall->xrepeat  = tempxrepeat;
                    pWall->yrepeat  = tempyrepeat;
                    pWall->cstat    = tempcstat;

                    if ((oCstat & kWallSwap) && !(tempcstat & kWallSwap))
                        pWall->cstat |= kWallSwap;

                    TranslateWallToSector();
                    if (sector[searchsector].type > 0 && (oCstat & kWallMoveMask))
                    {
                        pWall->cstat &= ~(kWallMoveForward | kWallMoveReverse);
                        if (oCstat & kWallMoveForward)
                        {
                            pWall->cstat |= kWallMoveForward;
                        }
                        else
                        {
                            pWall->cstat |= kWallMoveReverse;
                        }
                    }

                    if (sectorofwall(tempidx) < 0
                        || getWallLength(tempidx) != getWallLength(searchwall))
                            fixrepeats(searchwall);
                }

                return PROC_OKUB;
            }

            if (IsHoverSector())
            {
                pSect = &sector[searchsector];

                if (IS_FLOOR)
                {
                    pSect->floorpicnum      = temppicnum;
                    pSect->floorshade       = tempshade;
                    pSect->floorpal         = temppal;

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            pSect->visibility       = cpysector.visibility;
                            pSect->floorxpanning    = tempxrepeat;
                            pSect->floorypanning    = tempyrepeat;

                            oCstat = pSect->floorstat;
                            pSect->floorstat = tempcstat;
                            if (oCstat & kSectSloped)
                                pSect->floorstat |= kSectSloped;
                            break;
                    }

                    if (isSkySector(searchsector, searchstat))
                    {
                        Sky::FixPan(searchsector, searchstat, FALSE);
                        Sky::Setup(searchsector, searchstat, pSect->floorshade, pSect->floorpal, pSect->floorpicnum, -1, -1, FALSE);
                    }
                }
                else
                {
                    pSect->ceilingpicnum    = temppicnum;
                    pSect->ceilingshade     = tempshade;
                    pSect->ceilingpal       = temppal;

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            pSect->visibility       = cpysector.visibility;
                            pSect->ceilingxpanning  = tempxrepeat;
                            pSect->ceilingypanning  = tempyrepeat;

                            oCstat = pSect->ceilingstat;
                            pSect->ceilingstat = tempcstat;
                            if (oCstat & kSectSloped)
                                pSect->ceilingstat |= kSectSloped;
                            break;
                    }

                    if (isSkySector(searchsector, searchstat))
                    {
                        Sky::FixPan(searchsector, searchstat, FALSE);
                        Sky::Setup(searchsector, searchstat, pSect->ceilingshade, pSect->ceilingpal, pSect->ceilingpicnum, -1, -1, FALSE);
                    }
                }

                return PROC_OKUB;
            }

            if (IsHoverSprite())
            {
                pSpr = &sprite[searchwall];

                pSpr->picnum    = temppicnum;
                pSpr->shade     = tempshade;
                pSpr->pal       = temppal;

                if (somethingintab == searchstat)
                {
                    pSpr->xrepeat = tempxrepeat;
                    pSpr->yrepeat = tempyrepeat;
                    pSpr->cstat = tempcstat;

                    if ((pSpr->cstat & kSprRelMask) == kSprSloped)
                    {
                        pSpr->xoffset   = tempxoffset;
                        pSpr->yoffset   = tempyoffset;
                    }
                }

                clampSprite(pSpr);
                return PROC_OKUB;
            }
        }
    }

    if (ED2D || alt)
    {
        if (IsHoverWall())
        {
            pWall = &wall[searchwall];
            switch (somethingintab)
            {
                case OBJ_WALL:
                case OBJ_MASKED:
                    pWall->type     = temptype;
                    pWall->extra    = tempextra;
                    ok              = 1;
                    break;
            }
        }

        if (IsHoverSector())
        {
            pSect = &sector[searchsector];
            switch (somethingintab)
            {
                case OBJ_FLOOR:
                case OBJ_CEILING:
                case OBJ_SECTOR:
                    pSect->type     = temptype;
                    pSect->extra    = tempextra;
                    ok              = 1;

                    if (pSect->extra > 0)
                    {
                        CleanUp();
                        findSectorMarker(searchsector, &nMark0, &nMark1);

                        if (nMark0 >= 0)
                        {
                            getWallCoords(pSect->wallptr, &sprite[nMark0].x, &sprite[nMark0].y);
                            ChangeSpriteSect(nMark0, searchsector);
                        }

                        if (nMark1 >= 0)
                        {
                            getWallCoords(pSect->wallptr, &sprite[nMark1].x, &sprite[nMark1].y);
                            ChangeSpriteSect(nMark1, searchsector);
                        }
                    }
                    break;
            }

        }

        if (IsHoverSprite())
        {
            pSpr = &sprite[searchwall];
            pSpr->type  = temptype;
            pSpr->extra = tempextra;
            ok          = 1;
        }

        if (ok)
        {
            scrSetMessage("X-properties %s.", (tempextra > 0) ? "pasted" : "cleared");
            CleanUp();

            return PROC_OKUB;
        }

        scrSetMessage("Clipboard object is not a %s!", GetHoverName());
        return PROC_FAILB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_ESC(char key, char ctrl, char shift, char alt)
{
    int i = 0;
    if (gJoinSector >= 0)
    {
        gJoinSector = -1;
        scrSetMessage("Sector join/split mode disabled.");
        return PROC_OKB;
    }
    else if (sectorToolDisableAll(0))
    {
        scrSetMessage("Sector tools disabled.");
        return PROC_OKB;
    }
    else
    {
        if (highlightcnt > 0) hgltReset(kHgltPoint), i = 1;
        if (ED3D && highlightsectorcnt > 0) hgltReset(kHgltSector), i = 1;
        if (gListGrd.Length() > 0) gListGrd.Clear(), i = 1;
        if (i == 1)
        {
            scrSetMessage("Highlight reset.");
            return PROC_OKB;
        }
    }

    xmpMenuProcess();
    keyClear();
    return PROC_OK;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_TAB(char key, char ctrl, char shift, char alt)
{
    int i;

    if (alt)
        return PROC_OK;

    if (IS_EMPTY)
    {
        scrSetMessage("Clipboard buffer cleared.");
        somethingintab = OBJ_NONE;
        return PROC_OKB;
    }

    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            i = tempidx     = searchwall;
            temppicnum      = (searchstat == OBJ_MASKED) ? wall[i].overpicnum : wall[i].picnum;
            tempshade       = wall[i].shade;
            temppal         = wall[i].pal;
            tempxrepeat     = wall[i].xrepeat;
            tempyrepeat     = wall[i].yrepeat;
            tempxoffset     = wall[i].xpanning;
            tempyoffset     = wall[i].ypanning;
            tempcstat       = wall[i].cstat;
            tempextra       = wall[i].extra;
            temptype        = wall[i].type;

            cpywall = wall[i];
            if (wall[i].extra > 0)
                cpyxwall = xwall[wall[i].extra];

            break;
        case OBJ_SPRITE:
            i = tempidx     = searchwall;
            temppicnum      = sprite[i].picnum;
            tempshade       = sprite[i].shade;
            temppal         = sprite[i].pal;
            tempxrepeat     = sprite[i].xrepeat;
            tempyrepeat     = sprite[i].yrepeat;
            tempxoffset     = sprite[i].xoffset;
            tempyoffset     = sprite[i].yoffset;
            tempcstat       = sprite[i].cstat;
            tempextra       = sprite[i].extra;
            temptype        = sprite[i].type;

            cpysprite = sprite[i];
            if (sprite[i].extra > 0)
                cpyxsprite = xsprite[sprite[i].extra];

            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            i = tempidx     = searchsector;
            temptype        = sector[i].type;
            switch (searchstat)
            {
                case OBJ_FLOOR:
                    temppicnum      = sector[i].floorpicnum;
                    tempshade       = sector[i].floorshade;
                    temppal         = sector[i].floorpal;
                    tempxrepeat     = sector[i].floorxpanning;
                    tempyrepeat     = sector[i].floorypanning;
                    tempcstat       = sector[i].floorstat;
                    tempextra       = sector[i].extra;
                    break;
                default:
                    temppicnum      = sector[i].ceilingpicnum;
                    tempshade       = sector[i].ceilingshade;
                    temppal         = sector[i].ceilingpal;
                    tempxrepeat     = sector[i].ceilingxpanning;
                    tempyrepeat     = sector[i].ceilingypanning;
                    tempcstat       = sector[i].ceilingstat;
                    tempextra       = sector[i].extra;
                    break;
            }

            if (ED2D)
                searchstat = OBJ_SECTOR; // for names

            cpysector = sector[searchsector];
            if (sector[searchsector].extra > 0)
                cpyxsector = xsector[sector[searchsector].extra];

            break;
    }

    somethingintab = (char)searchstat;
    scrSetMessage("%s[%d] copied in clipboard.", GetHoverName(), i);
    return PROC_OKB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_DELETE(char key, char ctrl, char shift, char alt)
{
    if (ctrl)
    {
        if (ED2D)
            searchsector = sectorhighlight;
        
        if (searchsector < 0)
            return PROC_FAILB;

        ProcessSectors(sectDelete, -1, 0, BMODE, buffer);

        scrSetMessage("%s deleted.", buffer);
        updatesector(posx, posy, &cursectnum);
        updatenumsprites();
        CleanUp();

        return PROC_OKUB;
    }
    
    if (IsHoverSprite())
    {
        ProcessSprites(sprDelete, 0, BMODE, buffer);
        scrSetMessage("%s deleted.", buffer);
        updatenumsprites();

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

char dlgGoToLocation(int* x, int* y, int* z, int* a)
{
    Window dialog(0, 0, 120, 120, "Goto location");
    
    Label *lXPOS           = new Label(4, 8, "XPOS:");
    EditNumber *eXPOS      = new EditNumber(40, 4, 70, 18, *x);
    
    Label *lYPOS           = new Label(4, 30, "YPOS:");
    EditNumber *eYPOS      = new EditNumber(40, 26, 70, 18, *y);
    
    Label *lZPOS           = new Label(4, 52, "ZPOS:");
    EditNumber *eZPOS      = new EditNumber(40, 48, 70, 18, *z);
    
    Label *lANG            = new Label(4, 74, "ANG:");
    EditNumber *eANG       = new EditNumber(40, 70, 70, 18, *a);
    
    dialog.Insert(lXPOS); dialog.Insert(eXPOS);
    dialog.Insert(lYPOS); dialog.Insert(eYPOS);
    dialog.Insert(lZPOS); dialog.Insert(eZPOS);
    dialog.Insert(lANG);  dialog.Insert(eANG);
    
    if (ShowModal(&dialog) == mrCancel)
        return 0;
    
    *x = ClipRange(eXPOS->value, -boardWidth, +boardWidth);
    *y = ClipRange(eYPOS->value, -boardHeight, +boardHeight);
    *z = eZPOS->value;
    *a = eANG->value & kAngMask;
    
    return 1;
}

static char edKeyProcShared_KEY_HOME(char key, char ctrl, char shift, char alt)
{
    int nID, nType, i;
    int x, y, z, a;
    
    NAMED_TYPE searchFor[] =
    {
        OBJ_SPRITE,     gSearchStatNames[OBJ_SPRITE],
        OBJ_WALL,       gSearchStatNames[OBJ_WALL],
        OBJ_SECTOR,     gSearchStatNames[OBJ_SECTOR],
        OBJ_NONE,       "Location..."
    };

    if (ctrl)
    {
        while( 1 )
        {
            if ((nType = showButtons(searchFor, LENGTH(searchFor), "Go to...")) == mrCancel)
                return PROC_FAIL;

            nType -= mrUser;
            
            if (nType == OBJ_NONE)
            {
                x = posx, y = posy, z = posz;
                a = ang;
                
                if (dlgGoToLocation(&x, &y, &z, &a))
                {
                    updatesectorz(x, y, posz, &cursectnum), posx = x, posy = y, posz = z, ang = a;
                    scrSetMessage("Go to X:%d Y:%d Z:%d A:%d Sector=%d", x, y, z, ang, cursectnum);
                    clampCamera();
                    
                    return PROC_OKB;
                }
            }
            else
            {
                sprintf(buffer, "Goto %s #", gSearchStatNames[nType]);
                if ((nID = GetNumberBox(buffer, 0, -1)) >= 0)
                    break;
            }
        }

        switch(nType)
        {
            case OBJ_SPRITE:
                i = kMaxSprites;
                while(--i >= 0)
                {
                    if (sprite[i].index == nID && sprite[i].statnum < kMaxStatus)
                    {
                        posx = sprite[i].x; posy = sprite[i].y;
                        posz = sprite[i].z; ang = sprite[i].ang;
                        cursectnum = sprite[i].sectnum;
                        break;
                    }
                }
                if (i < 0) nID = -1;
                break;
            case OBJ_SECTOR:
                if (nID < numsectors)
                {
                    getWallCoords(sector[nID].wallptr, &posx, &posy);
                    posz = getflorzofslope(nID, posx, posy);
                    cursectnum = nID;
                    break;
                }
                nID = -1;
                break;
            case OBJ_WALL:
                if (nID < numwalls)
                {
                    i = sectorofwall(nID);
                    getWallCoords(nID, &posx, &posy);
                    posz = getflorzofslope(i, posx, posy);
                    cursectnum = i;
                    break;
                }
                nID = -1;
                break;
        }
        
        if (nID >= 0)
           scrSetMessage("Go to %s #%d", gSearchStatNames[nType], nID);
        else
            scrSetMessage ("%s not found - Sir Not Appearing In This Film", gSearchStatNames[nType]);
        
        clampCamera();

        return (nID >= 0) ? PROC_OKB : PROC_FAILB;
    }
	
/* 	if (alt)  //  in case game crash debug feature
	{
		
		Window dialog(0, 0, 180, 100, "Goto position");
		
        Label *lXPOS           = new Label(4, 8, "XPOS:");
        EditNumber *eXPOS      = new EditNumber(40, 4, 100, 18, posx);
		
        Label *lYPOS           = new Label(4, 30, "YPOS:");
        EditNumber *eYPOS      = new EditNumber(40, 26, 100, 18, posy);
		
        Label *lANG            = new Label(4, 52, "ANG:");
        EditNumber *eANG       = new EditNumber(40, 48, 100, 18, ang);
		
		dialog.Insert(lXPOS); dialog.Insert(eXPOS);
		dialog.Insert(lYPOS); dialog.Insert(eYPOS);
		dialog.Insert(lANG);  dialog.Insert(eANG);
		
        if (ShowModal(&dialog) == mrCancel)
        return PROC_FAILB;
	
        int x = eXPOS->value;
        int y = eYPOS->value;
        short nSector = cursectnum;

        updatesector(x, y, &nSector);
		
        if (nSector < 0)
		
		{
         scrSetMessage("Coordinates X=%d Y=%d have no sector", x, y);
         return PROC_FAILB;
		}
		
        posx = x; posy = y; posz = sector[nSector].floorz; ang = eANG->value & 2047;
        cursectnum = nSector;
        clampCamera();
        scrSetMessage("Goto: X-POS=%d Y-POS=%d ANG=%d Sector=%d", x, y, ang, nSector);
        return PROC_OKUB;
	} */
 
    if (shift)
    {
        if (gPreview.ShowMenu())
        {
            if (gPreviewMode) gPreview.Stop();
            gPreview.Start();
        }
    }
    else if (!gPreviewMode) gPreview.Start();
    else gPreview.Stop();

    return PROC_OK;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_SCROLLOCK(char key, char ctrl, char shift, char alt)
{
    updatesector(posx, posy, &cursectnum);

    if (cursectnum >= 0)
    {
        setStartPos(posx, posy, posz, ang, gMisc.forceEditorPos);
        scrSetMessage("Set start position at x=%d, y=%d, z=%d, in sector #%d", startposx, startposy, startposz, startsectnum);
        return PROC_OKUB;
    }

    scrSetMessage("Cannot set start position outside!");
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_COMMA(char key, char ctrl, char shift, char alt)
{
    char dir = (key == KEY_PERIOD); spritetype* pSpr;
    int i = (shift) ? ((ctrl) ? 128 : 256) : 512;
    int j, flags = 0;

    if (ED2D)
        i = (dir == 0) ? -i :  i;
    else
        i = (dir == 0) ?  i : -i;


    if (ED2D && highlightsectorcnt > 0)
    {
        if ((j = hgltSectRotate(0x04, i)) > 0)
        {
            scrSetMessage("Rotate %d sectors by %d.", j, i);
            return PROC_OKUB;
        }

        return PROC_OK;
    }

    if (IsHoverSprite())
    {
        pSpr = &sprite[searchwall];

        if (!sprInHglt(pSpr->index))
        {
            i = (shift) ? 16 : 256;
            pSpr->ang = (dir) ? IncNext(pSpr->ang, i) : DecNext(pSpr->ang, i);
            if (!isMarkerSprite(pSpr->index)) pSpr->ang = pSpr->ang & kAngMask;

            scrSetMessage("%s #%d angle: %d", GetHoverName(), pSpr->index, pSpr->ang);
        }
        else
        {
            hgltSprRotate(i);
            scrSetMessage("Rotate %d sprite(s) by %d", hgltSprCount(), i);
        }

        return PROC_OKUB;
    }

    if (ED3D && IsHoverWall())
    {
        // temporary rotate
        viewRotateWallTiles(1);

        if (searchwall != searchwall2)  flags |= 0x20;
        if (key == KEY_COMMA)           flags |= 0x10;
        if (!shift)                     flags |= 0x01;
        if (ctrl)                       flags |= 0x04;

        i = AutoAlignWalls(searchwall, flags);
        scrSetMessage("%d walls affected.", i);

        // restore it now
        viewRotateWallTiles(0);

        return (i > 0) ? PROC_OKUB : PROC_FAILB;
    }

    return PROC_OK;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_PAD5(char key, char ctrl, char shift, char alt)
{
    if (ctrl)
    {
        horiz = 100;
        return PROC_OKB;
    }

    if (ED2D)
    {
        switch(searchstat)
        {
            case OBJ_SPRITE:
                if ((sprite[searchwall].cstat & kSprRelMask) >= kSprWall) break;
            case OBJ_WALL:
                searchsector = sectorhighlight;
                // no break
            case OBJ_FLOOR:
            case OBJ_CEILING:
                switch(gScreen2D.prefs.showMap)
                {
                    case 0: return PROC_OK;
                    case 1: searchstat = OBJ_FLOOR;     break;
                    case 2: searchstat = OBJ_CEILING;   break;
                }
                if (!isSkySector(searchsector, searchstat)) break;
                return PROC_OK;
        }
    }

    switch (searchstat)
    {
        case OBJ_SPRITE:
            ProcessSprites(sprSizeSet, 64, BMODE, buffer);
            scrSetMessage("reset repeat for %s", buffer);
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            searchwall = searchwall2;
            if (alt)
            {
                ProcessWalls(wallSizeAuto, 0, false, BMODE, buffer);
                scrSetMessage("auto repeat for %s", buffer);
            }
            else
            {
                ProcessWalls(wallSizeReset, 0, false, BMODE, buffer);
                scrSetMessage("reset panning/repeat for %s", buffer);
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            ProcessSectors((IS_FLOOR) ? floorPanSet : ceilPanSet, searchstat, 0, BMODE, buffer);
            scrSetMessage("reset panning for %s", buffer);
            if (isSkySector(searchsector, searchstat))
                Sky::SetPan(searchsector, searchstat, 0, 0, alt);
            break;
    }

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_PAD7(char key, char ctrl, char shift, char alt)
{
    walltype* pWall; spritetype* pSpr; sectortype* pSect;
    signed char chx, chy, isPan = 0, dir, step, batch;
    int vlx, vly, c = 0;
    
    step = (shift) ? 1 : 8;
    dir = (key == KEY_PADUP || key == KEY_PADRIGHT || key == KEY_PAD9);
    
    if (dir == 0)
        step = -step;

    chx = (key == KEY_PADLEFT || key == KEY_PADRIGHT);
    chy = (key == KEY_PADUP   || key == KEY_PADDOWN);
    if (key == KEY_PAD7 || key == KEY_PAD9)
        chx = chy = 1;

    if (ED2D)
    {
        switch(searchstat)
        {
            case OBJ_SPRITE:
                if ((sprite[searchwall].cstat & kSprRelMask) >= kSprWall) break;
            case OBJ_WALL:
                searchsector = sectorhighlight;
                // no break
            case OBJ_FLOOR:
            case OBJ_CEILING:
                switch(gScreen2D.prefs.showMap)
                {
                    case 0: return PROC_OK;
                    case 1: searchstat = OBJ_FLOOR;     break;
                    case 2: searchstat = OBJ_CEILING;   break;
                }
                if (!isSkySector(searchsector, searchstat)) break;
                return PROC_OK;
        }
    }

    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            
            if (!ctrl)
                searchwall = searchwall2;
            
            pWall = &wall[searchwall];
            step = -step;
            
            // temporary rotate
            viewRotateWallTiles(1);

            if (ctrl)
            {
                strcpy(buffer, "panning");
                if (chx) c = ProcessWalls(wallPanXChg, step, false, BMODE, buffer2);
                if (chy) c = ProcessWalls(wallPanYChg, step, false, BMODE, buffer2);
                vlx = pWall->xpanning, vly = pWall->ypanning;
                isPan = 1;
            }
            else
            {
                strcpy(buffer, "repeat");
                if (chx) c = ProcessWalls(wallSizeXChg, step, false, BMODE, buffer2);
                if (chy) c = ProcessWalls(wallSizeYChg, step, false, BMODE, buffer2);
                vlx = pWall->xrepeat, vly = pWall->yrepeat;
            }

            // restore it now
            viewRotateWallTiles(0);

            break;
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            if (alt)
            {
                strcpy(buffer, "pos");
                if (chx) c = ProcessSprites(sprPosXChg, step, BMODE, buffer2);
                if (chy) c = ProcessSprites(sprPosYChg, step, BMODE, buffer2);
                vlx = pSpr->x, vly = pSpr->y;
            }
            else if (ctrl)
            {
                strcpy(buffer, "offset");
                if (chx) c = ProcessSprites(sprOffsXChg, step, BMODE, buffer2);
                if (chy) c = ProcessSprites(sprOffsYChg, step, BMODE, buffer2);
                vlx = pSpr->xoffset, vly = pSpr->yoffset;
            }
            else
            {
                strcpy(buffer, "repeat");
                if (chx) c = ProcessSprites(sprSizeXChg, step, BMODE, buffer2);
                if (chy) c = ProcessSprites(sprSizeYChg, step, BMODE, buffer2);
                vlx = pSpr->xrepeat, vly = pSpr->yrepeat;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            
            batch = (!alt && !isSkySector(searchsector, searchstat)) ? BMODE : 0;
            pSect = &sector[searchsector];
            strcpy(buffer, "panning");
            isPan = 1;
            
            if (IS_FLOOR)
            {
                if (chx) c = ProcessSectors(floorPanXChg, searchstat, step, batch, buffer2);
                if (chy) c = ProcessSectors(floorPanYChg, searchstat, step, batch, buffer2);
                vlx = pSect->floorxpanning, vly = pSect->floorypanning;
            }
            else
            {
                if (chx) c = ProcessSectors(ceilPanXChg, searchstat, step, batch, buffer2);
                if (chy) c = ProcessSectors(ceilPanYChg, searchstat, step, batch, buffer2);
                vlx = pSect->ceilingxpanning, vly = pSect->ceilingypanning;
            }

            if (isSkySector(searchsector, searchstat))
            {
                if (chy)
                    Sky::SetPan(searchsector, searchstat, 0, vly, alt);

                if (chx)
                {
                    Sky::FixPan(searchsector, searchstat, alt);     // fix panning first
                    Sky::Rotate(dir);                               // rotate global sky
                }
                
                return PROC_OKB|PROC_UNDO_ADD;
            }
            
            if (alt)
            {
                IDLIST visited(1);
                c = AutoAlignSectors(searchsector, searchstat, &visited);
                return (c > 0) ? PROC_OKUB : PROC_FAILB;
            }
            
            break;
    }
    
    if (c > 0)
    {
        if (c > 1)
        {
            *buffer3 = '\0';
            
            if (chx)
            {
                strcpy(buffer3, "X");
                if (chy)
                    strcat(buffer3, "/");
            }
            
            if (chy)
                strcat(buffer3, "Y");
            
            scrSetMessage("change %s %s by %+d for %s", buffer3, buffer, step, buffer2);
        }
        else
            scrSetMessage("%s x%s: %d  y%s: %d", buffer2, buffer, vlx, buffer, vly);
        
        return (isPan) ? PROC_OKB|PROC_UNDO_ADD : PROC_OKUB;
    }
    
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_PADENTER(char key, char ctrl, char shift, char alt)
{
    int i = 0;

    if (ED23)
    {
        if (keystatus[KEY_PADPERIOD])
        {
            i = ED3D, i |=0x80;
            xmpSetEditMode(i);
        }
        else if (ED3D) ED23 |= 0x10;
        else if (ED2D) ED23 |= 0x08;
    }
    else if (!keystatus[KEY_PADPERIOD])
    {
        if (ED2D)
        {
            updatesector(posx, posy, &cursectnum);
            if (gNoclip || cursectnum >= 0)
            {
                xmpSetEditMode(0x01);
            }
            else
            {
                scrSetMessage("Arrow must be inside a sector before entering 3D mode.");
            }
        }
        else
        {
            xmpSetEditMode(0x00);
        }
    }
    else
    {
        xmpSetEditMode(0x02);
    }

    keystatus[KEY_PADENTER]  = 0;
    keyClear();
    key = 0;

    return PROC_OK;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_B(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; walltype* pWall;
    int i, j, k, f;

    if (ctrl)
    {
        clipmovemask2d = clipmovemask3d = (clipmovemask3d == 0) ? BLOCK_MOVE : 0;
        scrSetMessage("Block movement is %s", onOff(clipmovemask3d));
        return PROC_OKB;
    }

    switch (searchstat)
    {
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            ProcessSprites((pSpr->cstat & kSprBlock) ? sprCstatRem : sprCstatAdd, kSprBlock, BMODE, buffer);
            scrSetMessage("%s %s blocking movement", buffer, isNot(pSpr->cstat & kSprBlock));
            return PROC_OKUB;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            if (ED2D) break;
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            pWall = &wall[searchwall];
            if (ProcessWalls((pWall->cstat & kWallBlock) ? wallCstatRem : wallCstatAdd_Block, kWallBlock, !shift, BMODE, buffer))
            {
                scrSetMessage("%s %s blocking movement", buffer, isNot(pWall->cstat & kWallBlock));
                return PROC_OKUB;
            }
            break;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_D(char key, char ctrl, char shift, char alt)
{
    int ex[4], ey[4], xs, ys, cd, i;
    char* msg = buffer;
    spritetype* pSpr;

    if (IsHoverSprite())
    {
        pSpr = &sprite[searchwall];
        msg += sprintf(buffer, "Sprite #%d", pSpr->index);

        if (!ctrl)
        {
            if (alt)
            {
                i = pSpr->ang, pSpr->ang = 1536, cd = pSpr->clipdist;
                if (GetVoxSpriteExtents(pSpr, &ex[0], &ey[0], &ex[1], &ey[1], &ex[2], &ey[2], &ex[3], &ey[3]))
                {
                    xs = klabs(ex[1] - ex[0]) >> 4;
                    ys = klabs(ey[2] - ey[1]) >> 4;
                }
                else
                {
                    GetSpriteExtents(pSpr, &ex[0], &ey[0], &ex[1], &ey[1], &ex[2], &ey[2], &ex[3], &ey[3]);
                    xs = klabs(ex[1] - ex[0]) >> 4;
                    ys = 0;
                }

                pSpr->ang = i;


                cd = ClipRange((xs+ys)>>1, 0, 255);

                sprintf(msg, " %s (%d - %d)", "clipdist", 0, 255);
                if ((i = dlgClipdist(buffer, pSpr->clipdist, cd, -1)) == -1)
                    return PROC_FAILB;
                
                ProcessSprites(sprClipdistSet, ClipHigh(i, 255), BMODE, buffer);
                scrSetMessage("%s clipdist: %d", buffer, pSpr->clipdist);
                return PROC_OKUB;
            }
            else
            {
                sprintf(msg, " %s (%d - %d)", "detail", 0, 4);
                if ((i = GetNumberBox(buffer, pSpr->detail, -1)) == -1)
                    return PROC_FAILB;
                
                ProcessSprites(sprDetailSet, ClipHigh(i, 4), BMODE, buffer);
                scrSetMessage("%s detail: %d", buffer, pSpr->detail);
                return PROC_OKUB;
            }

            return PROC_FAILB;
        }

        if (ctrl)
        {
            ProcessSprites((pSpr->flags & kHitagNoModel) ? sprFlagsRem : sprFlagsAdd, kHitagNoModel, BMODE, buffer);
            scrSetMessage("%s %s showing model", buffer, isNot((pSpr->flags & kHitagNoModel) == 0));
            return PROC_OKUB;
        }
    }

    return PROC_FAIL;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_E(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; sectortype* pSect;
    int i;

    if (ED2D && !IsHoverSprite() && !helperTranslateToSector())
        return PROC_FAILB;

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];
        
        if (IS_FLOOR)
        {
            i = (pSect->floorstat & kSectExpand);
            ProcessSectors(i ? floorStatRem : floorStatAdd, searchstat, kSectExpand, BMODE, buffer);
        }
        else
        {
            i = (pSect->ceilingstat & kSectExpand);
            ProcessSectors(i ? ceilStatRem : ceilStatAdd, searchstat, kSectExpand, BMODE, buffer);
        }
        
        scrSetMessage("%s texture %s expanded", buffer, isNot(i != 0));
        return PROC_OKUB;
    }

    if (searchstat != OBJ_SPRITE)
        return PROC_FAILB;
    
    pSpr = &sprite[searchwall];
    if ((i = GetNumberBox("Enter statnum", pSpr->statnum, pSpr->statnum)) == pSpr->statnum)
        return PROC_OKB;

    ChangeSpriteStat(searchwall, ClipRange(i, 0, kMaxStatus - 1));
    scrSetMessage("sprite[%d].statnum = %d", searchwall, pSpr->statnum);
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_I(char key, char ctrl, char shift, char alt)
{
    if (ctrl)
    {
        scrSetMessage("Show invisible sprites is %s", onOff(showinvisibility^=1));
        return PROC_OKB;
    }
    
    if (IsHoverSprite())
    {
        spritetype* pSpr = &sprite[searchwall];
        ProcessSprites((pSpr->cstat & kSprInvisible) ? sprCstatRem : sprCstatAdd, kSprInvisible, BMODE, buffer);
        scrSetMessage("%s %s invisible", buffer, isNot(pSpr->cstat & kSprInvisible));
        return PROC_OKUB;
    }
    
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_J(char key, char ctrl, char shift, char alt)
{
    int nSectA = gJoinSector;
    int nSectB = (ED2D) ? sectorhighlight : searchsector;
    int nWall = -1, numparts, s, e, ls, le;
    char joinMode = (shift && nSectA >= 0);

    if ((nSectA >= 0 && nSectA >= numsectors) || nSectB < 0)
        return PROC_FAILB;

    if (!shift && nSectA < 0)
    {
        if (ED3D)
        {
            switch(searchstat)
            {
                case OBJ_SPRITE:
                    nWall = searchwall2;
                    break;
                case OBJ_WALL:
                case OBJ_MASKED:
                    nWall = searchwall;
                    if (wall[nWall].nextwall >= 0
                        && (wall[wall[nWall].nextwall].cstat & kWallSwap))
                            nWall = searchwall2;
                    break;
                default:
                    nWall = searchwall;
                    break;
            }
        }
        else
        {
            nWall = linehighlight;
        }

        if (nWall < 0 || wall[nWall].nextsector < 0)
        {
            scrSetMessage("Must point near red wall.");
            return PROC_FAILB;
        }

        nSectA = nSectB;
        nSectB = wall[nWall].nextsector;
    }

    if (nSectA == nSectB)
    {
        if ((numparts = sectCountParts(nSectB)) > 1 && numsectors+1 < kMaxSectors)
        {
            getSectorWalls(nSectB, &s, &e);
            sectortype model = sector[nSectB];
            VOIDLIST wls(sizeof(walltype));

            while (s <= e)
            {
                loopGetWalls(s, &ls, &le);
                if (clockdir(ls) == 0 && loopInside(mousxplc, mousyplc, ls))
                {
                    s = ls;
                    do
                    {
                        wls.Add(&wall[s]);
                        s = wall[s].point2;
                    }
                    while(s != ls);

                    loopDelete(ls);
                    ls = insertLoop(-1, (walltype*)wls.First(), wls.Length(), &model);
                    nSectA = sectorofwall(ls); sectLoopTransfer(nSectB, nSectA);
                    sectAttach(nSectA);
                    numparts--;

                    scrSetMessage("Sector #%d part split (%d parts left)", nSectB, numparts);
                    updatesector(posx, posy, &cursectnum);
                    CleanUp();

                    return PROC_OKUB;
                }

                s = ++le;
            }
        }

        gJoinSector = -1;
        scrSetMessage("There is nothing to merge.");
        return PROC_FAILB;
    }

    if (nSectA < 0)
    {
        scrSetMessage("Sector join/split mode: press J on another sector.");
        gJoinSector = nSectB;
        return PROC_OKB;
    }

    if (redSectorMerge(nSectA, nSectB) == 0)
    {
        scrSetMessage("Sectors #%d and #%d are merged.", nSectA, nSectB);
        gJoinSector = (joinMode) ? numsectors-1 : -1;
        updatesector(posx, posy, &cursectnum);
        CleanUp();

        return PROC_OKUB;
    }

    gJoinSector = -1;
    scrSetMessage("Error merging sector.");
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_K(char key, char ctrl, char shift, char alt)
{
    int i;
    switch (searchstat)
    {
        case OBJ_SPRITE:
            i = sprite[searchwall].cstat & kSprMoveMask;
            ProcessSprites(sprCstatRem, kSprMoveMask, BMODE, buffer);
            switch (i)
            {
                case kSprMoveNone:
                    ProcessSprites(sprCstatAdd, kSprMoveForward, BMODE, buffer);
                    scrSetMessage("%s moves forward (blue).", buffer);
                    break;
                case kSprMoveForward:
                    ProcessSprites(sprCstatAdd, kSprMoveReverse, BMODE, buffer);
                    scrSetMessage("%s moves reverse (green).", buffer);
                    break;
                case kSprMoveReverse:
                    scrSetMessage("kinetic move disabled for %s.", buffer);
                    break;
            }
            return PROC_OKUB;
        case OBJ_WALL:
        case OBJ_MASKED:
            i = wall[searchwall].cstat & kWallMoveMask;
            ProcessWalls(wallCstatRem, kWallMoveMask, !shift, BMODE, buffer);
            switch (i)
            {
                case kWallMoveNone:
                    ProcessWalls(wallCstatAdd, kWallMoveForward, !shift, BMODE, buffer);
                    scrSetMessage("%s moves forward (blue).", buffer);
                    break;
                case kWallMoveForward:
                    ProcessWalls(wallCstatAdd, kWallMoveReverse, !shift, BMODE, buffer);
                    scrSetMessage("%s moves reverse (green).", buffer);
                    break;
                case kWallMoveReverse:
                    scrSetMessage("kinetic move disabled for %s.", buffer);
                    break;
            }
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_M(char key, char ctrl, char shift, char alt)
{
    int i;

    if (IsHoverSprite())
    {
        searchwall = searchwall2;
        searchstat = OBJ_WALL;
    }
    
    if (searchwall >= 0)
    {
        walltype* pWall = &wall[searchwall];
        ProcessWalls((pWall->cstat & kWallMasked) ? wallCstatRem : wallCstatAdd_Mask, kWallMasked, !shift, BMODE, buffer);
        scrSetMessage("%s %s masked", buffer, isNot(pWall->cstat & kWallMasked));
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_Q(char key, char ctrl, char shift, char alt)
{
    int nID = GetHoverID();

    // connect one object with the one from clipboard via RX/TX

    if (somethingintab == OBJ_NONE)
    {
        scrSetMessage("Clipboard is empty!");
        return PROC_FAILB;
    }

    if ((shift && xsysConnect2(somethingintab, tempidx, searchstat, nID) < 0)
        || (!shift && xsysConnect(somethingintab, tempidx, searchstat, nID) < 0))
                return PROC_FAILB;

    scrSetMessage("Objects connected.");
    CleanUpMisc();

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_R(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; walltype* pWall; sectortype* pSect;
    int i;

    if (ED2D
        && searchstat != OBJ_SPRITE && !helperTranslateToSector())
            return PROC_FAILB;

    // R (relative alignment, rotation)

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];
        if (IS_FLOOR)
        {
            i = pSect->floorstat & kSectRelAlign;
            ProcessSectors(i ? floorStatRem : floorStatAdd, searchstat, kSectRelAlign, BMODE, buffer);
        }
        else
        {
            i = pSect->ceilingstat & kSectRelAlign;
            ProcessSectors(i ? ceilStatRem : ceilStatAdd, searchstat, kSectRelAlign, BMODE, buffer);
        }
        
        scrSetMessage("%s %s relative", buffer, isNot(i == 0));
        return PROC_OKUB;
    }

    if (IS_WALL)
    {
        pWall = &wall[searchwall];
        ProcessWalls((pWall->cstat & kWallRotate90) ? wallCstatRem : wallCstatAdd, kWallRotate90, false, BMODE, buffer);
        scrSetMessage("tile %s rotated 90deg for %s", isNot(pWall->cstat & kWallRotate90), buffer);
        return PROC_OKUB;
    }
    
    if (IsHoverSprite())
    {
        pSpr = &sprite[searchwall];
        i = pSpr->cstat & kSprRelMask;
        ProcessSprites(sprCstatRem, kSprRelMask, BMODE, buffer);
        
        switch (i)
        {
            case kSprFace:
                ProcessSprites(sprCstatAdd, kSprWall, BMODE, buffer);
                scrSetMessage("%s wall oriented", buffer);
                break;
            case kSprWall:
                ProcessSprites(sprCstatAdd, kSprFloor, BMODE, buffer);
                scrSetMessage("%s floor oriented", buffer);
                break;
            default:
                scrSetMessage("%s face oriented", buffer);
                break;
        }
        
        ProcessSprites(spr_Floor_OneSide_HandleFlip, posz, BMODE, NULL);
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_U(char key, char ctrl, char shift, char alt)
{
    int nVis;
	int nShade, nID;
	nID = GetHoverID();
    
 	if (alt && ED3D)
    {
		nID = GetHoverID();

        nShade = getShadeOf(searchstat, nID);
        if ((nShade = GetNumberBox("Shade", nShade, -1)) == -1)
             return PROC_FAILB;

        nShade = ClipRange(nShade, -128, 127);
        setShadeOf(nShade, searchstat, nID);
		
		scrSetMessage("%s #%d shade: %d", GetHoverName(), nID, nShade);
        return PROC_OKUB;
    }
    if (IsHoverWall())
        TranslateWallToSector();
    else if (IsHoverSprite())
        searchsector = sprite[searchwall].sectnum;

    if (searchsector < 0)
        return PROC_FAILB;

    if (ctrl)
    {
        nVis = sector[searchsector].visibility;
        if ((nVis = GetNumberBox("Visibility", nVis, -1)) == -1)
            return PROC_FAILB;

        nVis = ClipRange(nVis, 0, 255);
        ProcessSectors(sectVisibilitySet, -1, nVis, BMODE | 0x02, buffer);
        scrSetMessage("%s visibility = %d", buffer, nVis);
        return PROC_OKUB;
    }

    XSECTOR* pXSect = &xsector[GetXSector(searchsector)];
    ProcessSectors(sectUnderwaterSet, -1, !pXSect->underwater, BMODE, buffer);
    scrSetMessage("%s %s underwater", buffer, isNot(pXSect->underwater));
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_W(char key, char ctrl, char shift, char alt)
{
    int nXSect, nWave, i, j;
    int x1, y1, x2, y2;

    if (ED3D)
    {
        // change lighting waveform
        nXSect = GetXSector(searchsector);
        nWave = xsector[nXSect].shadeWave;
        do
        {
            nWave = IncRotate(nWave, 12);
        }
        while (gWaveNames[nWave] == NULL);
        ProcessSectors(sectFXWaveSet, -1, nWave, BMODE, buffer);
        ProcessSectors(sectFXSetShadeFlags, -1, 1, BMODE);
        scrSetMessage("%s lighting wave: %s", buffer, gWaveNames[nWave]);
        CleanUp();

        return PROC_OKUB;
    }

    if (gCmtPrefs.enabled)
    {
        gScreen2D.GetPoint(searchx, searchy, &x1, &y1), x2 = x1, y2 = y1;
        i = cmthglt;

        if (i >= 0)
        {
            // body (edit comment)
            if ((i & 0xc000) == 0) gCommentMgr.ShowDialog(x1, y1, x2, y2, i);
            else gCommentMgr.ShowBindMenu(i & 0x3FFF, x1, y1); // tail (bind, unbind, delete)
            return PROC_OKUB;
        }

        if (gCommentMgr.ShowDialog(x1, y1, x2, y2) >= 0)
        {
            scrSetMessage("New comment created.");
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_X(char key, char ctrl, char shift, char alt)
{
    int flags = (key == KEY_X);
    int nSect, nWall;
    int s, e, cx, cy;
    int i = 0;

    if (ED2D)
    {
        if (ctrl)
        {
            if (hgltSectInsideBox(mousxplc, mousyplc))
            {
                flags |= 0x04; // flip with highlight outer loops
                i = hgltSectFlip(flags);
            }
            else if (sectorhighlight >= 0)
            {
                if (!isIslandSector(sectorhighlight))
                {
                    scrSetMessage("You cannot flip sectors with connections");
                    return PROC_FAILB;
                }

                flags |= 0x02; // flip with outer loop of red sector
                getSectorWalls(sectorhighlight, &s, &e); midPointLoop(s, e, &cx, &cy);
                sectFlip(sectorhighlight, cx, cy, flags, 0);
                i = 1;
            }

            if (i > 0)
                scrSetMessage("%d sectors were flipped-%s", i, (flags & 0x01) ? "X" : "Y");

            return (i) ? PROC_OKUB : PROC_FAILB;
        }
    }

    if (key != KEY_X)
        return PROC_FAILB;

    if (alt & 0x01)
    {
        if (highlightsectorcnt > 0)
        {
            // disable auto align for sectors in a highlight
            for (i = 0; i < highlightsectorcnt; i++)
                sector[highlightsector[i]].alignto = 0;

            scrSetMessage("Auto slope disabled for all highlighted sectors.");
            return PROC_OKUB;
        }

        return PROC_FAILB;
    }


    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            nSect = sectorofwall(searchwall);
            nWall = sector[nSect].wallptr;
            if (sector[nSect].alignto)
            {
                if (nWall == searchwall || nWall + sector[nSect].alignto == searchwall)
                {
                    scrSetMessage("Sector #%d auto slope disabled", nSect);
                    sector[nSect].alignto = 0;
                    return PROC_OKUB;
                }
            }
            else if (nWall == searchwall)
            {
                scrSetMessage("You cannot select first wall for auto slope");
                return PROC_FAILB;
            }

            if (wall[searchwall].nextwall >= 0)
            {
                setAligntoWall(nSect, searchwall);

                if (sector[nSect].alignto)
                {
                    scrSetMessage("Sector #%d will align to wall #%d (%d)", nSect, searchwall, sector[nSect].alignto);
                    return PROC_OKUB;
                }
            }
            else
            {
                scrSetMessage("Must select the red wall!");
                return PROC_FAILB;
            }
            // no break
        case OBJ_FLOOR:
        case OBJ_CEILING:
            if (nSect < 0)
                nSect = searchsector;

            if (sector[nSect].alignto)
            {
                scrSetMessage("Sector %d auto-alignment disabled!", nSect);
                sector[nSect].alignto = 0;
                return PROC_OKUB;
            }
            break;
    }

    return PROC_FAILB;
}

static char edKeyProcShared_KEY_Z(char key, char ctrl, char shift, char alt)
{
    char isRedo; int nStat;

    if (!gPreviewMode && ctrl)
    {
        isRedo = (shift || key == KEY_A);
        strcpy(buffer, (isRedo) ? "Redo" : "Undo");
        if ((nStat = gMapSnapshot.Switch(isRedo)) > 0)
        {
            scrSetMessage("%s: restored revision #%d.", buffer, nStat);
            return PROC_OKB;
        }

        switch(nStat)
        {
            case 0:
                scrSetMessage("%s: no more revisions left!", buffer);
                return PROC_FAILB;
            default:
                Alert("%s: error occured (code: %d).", buffer, nStat);
                return PROC_FAILB;
        }
    }

    return PROC_FAIL;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_F5(char key, char ctrl, char shift, char alt)
{
    int i = 1;
    int nSect;

    if (key == KEY_F6
        && ED3D && IsHoverSector())
            searchstat = OBJ_WALL;

    switch (searchstat)
    {
        case OBJ_SPRITE:
            hudSetLayout(&gMapedHud, kHudLayoutFull);
            if (key == KEY_F6) EditSpriteData(searchwall, !ctrl);
            else
            {
                nSect = (ED2D) ? sectorhighlight : sprite[searchwall].sectnum;
                if (nSect >= 0)
                {
                    if (shift) EditSectorLighting(nSect);
                    else EditSectorData(nSect, !ctrl);
                }
            }
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            hudSetLayout(&gMapedHud, kHudLayoutFull);
            if (key == KEY_F6) EditWallData(searchwall, !ctrl);
            else
            {
                i = wall[searchwall].nextwall;
                if (ED2D) nSect = sectorhighlight;
                else nSect = sectorofwall((alt || i < 0) ? searchwall : i);

                if (nSect < 0) i = 0;
                else if (shift) EditSectorLighting(nSect);
                else EditSectorData(nSect, !ctrl);
            }
            break;
        case OBJ_CEILING:
        case OBJ_FLOOR:
            if (key == KEY_F5)
            {
                hudSetLayout(&gMapedHud, kHudLayoutFull);
                if (!shift) EditSectorData(searchsector, !ctrl);
                else EditSectorLighting(searchsector);
                break;
            }
            // no break
        default:
            return PROC_FAILB;
    }

    if (i)
    {
        if (ED23)       hudSetLayout(&gMapedHud, gHudPrefs.layoutSPLIT, &gMouse);
        else if (ED2D)  hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
        else            hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);

        return PROC_OKU;
    }

    return PROC_FAIL;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_F10(char key, char ctrl, char shift, char alt)
{
    static int sum = 0, odata = 0;
    int i = 0;

    if (highlightsectorcnt > 0) i |= kHgltSector;
    if (highlightcnt > 0)       i |= kHgltPoint;
    if (i > 0)
    {
        hgltIsolateRorMarkers(i); hgltIsolatePathMarkers(i);
        hgltIsolateChannels(i);
        CleanUp();

        scrSetMessage("Objects isolated.");
        return PROC_OKUB;
    }

    if (IsHoverSprite())
    {
        spritetype *pSpr = &sprite[searchwall];
        GetXSprite(searchwall);

        switch (pSpr->type)
        {
            case kSoundAmbient:
            case kDudeModernCustom:
                auditSound(xsprite[pSpr->extra].data3, pSpr->type);
                break;
            case kGenSound:
                auditSound(xsprite[pSpr->extra].data2, pSpr->type);
                break;
            case kSoundSector:
                odata = (searchwall != sum) ? 0 : IncRotate(odata, 4);
                auditSound(getDataOf(odata, OBJ_SPRITE, searchwall), pSpr->type);
                break;
            case kSwitchToggle:
            case kSwitchOneWay:
                odata = (searchwall != sum) ? 0 : IncRotate(odata, 2);
                auditSound(getDataOf(odata, OBJ_SPRITE, searchwall), pSpr->type);
                break;
            case kSwitchCombo:
            case kThingObjectGib:
            case kThingObjectExplode:
            case 425:
            case 426:
            case 427:
                auditSound(xsprite[pSpr->extra].data4, pSpr->type);
                break;
            case kMarkerWarpDest:
                if (pSpr->statnum == kStatMarker) break;
                auditSound(xsprite[pSpr->extra].data4, pSpr->type);
                break;
            case kSoundPlayer:
                auditSound(xsprite[pSpr->extra].data1, pSpr->type);
                break;
        }

        sum = searchwall;
        return PROC_OK;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_F11(char key, char ctrl, char shift, char alt)
{
    int i = 0;

    if (!alt)
    {
        if (ED2D)
        {
            i = IncRotate(gScreen2D.prefs.showMap, 3);
            scrSetMessage("Show 2D map: %s", gShowMapNames[i]);
            gScreen2D.prefs.showMap = i;
            return PROC_OKB;
        }

        gMisc.pan = !gMisc.pan;
        scrSetMessage("Global panning and slope auto-align is %s", onOff(gMisc.pan));
        if (gMisc.pan)
            AlignSlopes();

        return PROC_OKB;
    }

    if (ctrl)
    {
        gHudPrefs.fontPack = IncRotate(gHudPrefs.fontPack, kHudFontPackMax);
        gMapedHud.SetMsgImp(128, "HUD font: %s", gHudFontNames[gHudPrefs.fontPack]);
        hudSetFontPack(&gMapedHud, gHudPrefs.fontPack, &gMouse);
        return PROC_OKB;
    }

    while( 1 )
    {
        if (ED23)
        {
            strcpy(buffer, "SPLIT");
            if (gHudPrefs.dynamicLayoutSPLIT) gHudPrefs.layoutSPLIT = kHudLayoutNone;
            else gHudPrefs.layoutSPLIT = IncRotate(gHudPrefs.layoutSPLIT, kHudLayoutMax);
            gHudPrefs.dynamicLayoutSPLIT = (gHudPrefs.layoutSPLIT == kHudLayoutDynamic);

            i = gHudPrefs.layoutSPLIT;
            if (gHudPrefs.dynamicLayoutSPLIT)
                gHudPrefs.layoutSPLIT = kHudLayoutFull;

            hudSetLayout(&gMapedHud, gHudPrefs.layoutSPLIT, &gMouse);
        }
        else if (ED2D)
        {
            strcpy(buffer, "2D");
            if (gHudPrefs.dynamicLayout2D) gHudPrefs.layout2D = kHudLayoutNone;
            else gHudPrefs.layout2D = IncRotate(gHudPrefs.layout2D, kHudLayoutMax);
            gHudPrefs.dynamicLayout2D = (gHudPrefs.layout2D == kHudLayoutDynamic);

            i = gHudPrefs.layout2D;
            if (gHudPrefs.dynamicLayout2D)
                gHudPrefs.layout2D = kHudLayoutFull;

            hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
        }
        else
        {
            strcpy(buffer, "3D");
            gHudPrefs.layout3D = IncRotate(gHudPrefs.layout3D, kHudLayoutMax);
            if (gHudPrefs.layout3D == kHudLayoutDynamic) continue;
            else hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);
            i = gHudPrefs.layout3D;
        }

        gMapedHud.SetMsgImp(128, "HUD layout (%s Mode): %s", buffer, gHudLayoutNames[i]);
        break;
    }

    return PROC_OKB;
}

static char edKeyProcShared_KEY_N(char key, char ctrl, char shift, char alt)
{
    switch(gBatchMode & 0x03)
    {
        case 0x00: gBatchMode = 0x01; break;
        case 0x01: gBatchMode = 0x03; break;
        default:   gBatchMode = 0x00; break;
    }
    
    scrSetMessage("Batch processing mode: %s", gBatchModeNames[gBatchMode]);
    return (gBatchMode) ? PROC_OKB : PROC_FAILB;
}

////////////////////////////////////////////////////
/***********************************************/
/** 3D MODE ONLY MAP EDITING INPUT FUNCTIONS  **/
/***********************************************/
//////////////////

static char edKeyProc3D_KEY_A(char key, char ctrl, char shift, char alt)
{
    int const nStep = 0x400;

    if (key == KEY_Z && ctrl)
        return PROC_FAIL;

    if (!alt)
        return PROC_FAIL;

    if (!gNoclip)
        zmode = 0;

    if (key == KEY_Z) kensplayerheight = ClipLow(kensplayerheight - nStep, 0);
    else kensplayerheight = ClipHigh(kensplayerheight + nStep, 0x10000 << 2);
    scrSetMessage("Camera height (gravity): %d", kensplayerheight >> 8);
    return PROC_OKB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_CAPSLOCK(char key, char ctrl, char shift, char alt)
{
    if ((shift & 0x01) == 0)
        return PROC_FAIL;

    gMouseLook.mode = !gMouseLook.mode;
    scrSetMessage("Mouse look is %s", onOff(gMouseLook.mode));
    return PROC_OKB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_BACKSPACE(char key, char ctrl, char shift, char alt)
{
    // reset cstat flags

    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            wall[searchwall].cstat = 0;
            scrSetMessage("Wall #%d cstat reset", searchwall);
            return PROC_OKUB;

        case OBJ_SPRITE:
            sprite[searchwall].cstat = kSprOrigin;
            scrSetMessage("Sprite #%d cstat reset", searchwall);
            return PROC_OKUB;

        case OBJ_FLOOR:
            sector[searchsector].floorstat = 0;
			sector[searchsector].floorslope = 0;
            scrSetMessage("Sector #%d floor cstat reset", searchsector);
            return PROC_OKUB;

        case OBJ_CEILING:
            sector[searchsector].ceilingstat = 0;
			sector[searchsector].ceilingslope = 0;
            scrSetMessage("Sector #%d ceiling cstat reset", searchsector);
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_INSERT(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr;
    int i;

    if (!IsHoverSprite())
        return PROC_FAILB;

    i = ProcessSprites(sprClone, 0, BMODE, buffer);
    scrSetMessage("%s duplicated and stamped.", buffer);

    if (i > 1)
    {
        for (i = 0; i < highlightcnt; i++)
        {
            if ((highlight[i] & 0xC000) == 0)
                continue;

            searchwall = highlight[i] & 0x3FFF;
            break;
        }

        searchit = 0;
        gObjectLock.Set(256); // give user some time to drag out new sprites
        gMapedHud.SetMsgImp(256, "Locked on %s #%d", GetHoverName(), searchwall);
    }

    return (i > 0) ? PROC_OKUB : PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PAGEDN(char key, char ctrl, char shift, char alt)
{
    int nStep = (shift) ? 0x100 : 0x400;
    char d = (key == KEY_PAGEUP);
    int z = 0x80000000, i;

    if (d)
        nStep = -nStep;

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector() && ctrl && alt)
    {
        ProcessSectors(sectOffsetZ, -1, nStep, BMODE, buffer);
        scrSetMessage("z-offset %s by %+d", buffer, nStep);
        if (gMisc.pan)
            AlignSlopes();

        return PROC_OKUB;
    }

    if (ctrl)
        nStep >>= 4;

    if (ctrl && !shift)
    {
        switch (searchstat)
        {
            case OBJ_CEILING:
                if (d) z = NextSectorNeighborZ(searchsector, sector[searchsector].ceilingz, 0, 0);
                else z = NextSectorNeighborZ(searchsector, sector[searchsector].ceilingz, 0, 1);
                ProcessSectors(ceilZSet, searchstat, z, BMODE);
                break;
            case OBJ_FLOOR:
                if (d) z = NextSectorNeighborZ(searchsector, sector[searchsector].floorz, 1, 0);
                else z = NextSectorNeighborZ(searchsector, sector[searchsector].floorz, 1, 1);
                ProcessSectors(floorZSet, searchstat, z, BMODE);
                break;
            case OBJ_SPRITE:
                ProcessSprites((d) ? sprPutOnCeil : sprPutOnFloor, 0, BMODE);
                z = sprite[searchwall].z;
                break;
        }
    }
    else if (alt)
    {
        switch (searchstat)
        {
            case OBJ_CEILING:
                z = dlgZHeightOff("height off floor / shift ceiling by", sector[searchsector].floorz, sector[searchsector].ceilingz, 0, key == KEY_PAGEDN);
                ProcessSectors(ceilZChg, searchstat, z, BMODE);
                z = sector[searchsector].ceilingz;
                break;
            case OBJ_FLOOR:
                z = dlgZHeightOff("height off ceiling / shift floor by", sector[searchsector].floorz, sector[searchsector].ceilingz, 1, key == KEY_PAGEDN);
                ProcessSectors(floorZChg, searchstat, z, BMODE);
                z = sector[searchsector].floorz;
                break;
            case OBJ_SPRITE:
                if (sprInHglt(searchwall))
                {
                    if (d)
                    {
                        strcpy(buffer, gSearchStatNames[OBJ_CEILING]);
                        hgltSprPutOnCeiling();
                    }
                    else
                    {
                        strcpy(buffer, gSearchStatNames[OBJ_FLOOR]);
                        hgltSprPutOnFloor();
                    }

                    scrSetMessage("%d sprite(s) put on %s keeping the shape", hgltSprCount(), buffer);
                    return PROC_OKUB;
                }
                break;
        }
    }
    else if (IsHoverSector())
    {
        if (sectInHglt(searchsector))
            ProcessSectors(IS_FLOOR ? floorZChg : ceilZChg, searchstat, nStep, BMODE);
        else
            ProcessSectors(IS_FLOOR ? floorZChgNext : ceilZChgNext, searchstat, nStep, BMODE);
        
        z = (IS_FLOOR)
            ? sector[searchsector].floorz : sector[searchsector].ceilingz;
    }
    else if (IsHoverSprite())
    {
        ProcessSprites(sprInHglt(searchwall) ? sprZChg : sprZChgNext, nStep, BMODE);
        z = sprite[searchwall].z;
    }


    if (z != 0x80000000)
    {
        scrSetMessage("%s #%d Z: %d", GetHoverName(), GetHoverID(), z);
        if (gMisc.pan && IsHoverSector())
            AlignSlopes();

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_RBRACE(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect = NULL;
    int nStep = (shift) ? 32 : 256;
    int nNext, x, y;
    
    if (key == KEY_RBRACE)
        nStep = -nStep;
    
    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];

        if (pSect->alignto)
        {
            scrSetMessage("Sector must be non auto-aligned!");
            return PROC_FAILB;
        }
        
        if (alt && key == KEY_RBRACE)
        {
            if ((nNext = wall[searchwall].nextsector) < 0)
                return PROC_FAILB;

            x = wall[searchwall].x;
            y = wall[searchwall].y;

            if (IS_FLOOR) alignflorslope(searchsector, x, y, getflorzofslope(nNext, x, y));
            else alignceilslope(searchsector, x, y, getceilzofslope(nNext, x, y));
            return PROC_OKUB;
        }
        
        if (ProcessSectors((IS_FLOOR) ? floorSlopeChg : ceilSlopeChg, searchstat, nStep, BMODE, buffer) > 1)
            scrSetMessage("adjust slope for %s by %d", buffer, nStep);
        else
            scrSetMessage("%s slope: %d", buffer, (IS_FLOOR) ? pSect->floorslope : pSect->ceilingslope);
        
        return PROC_OKUB;
    }
    
    if (IsHoverSprite())
    {
        if (ctrl || alt)
        {
            ProcessSprites((ctrl) ? sprSlopeSetFloor : sprSlopeSetCeil, 0, BMODE, buffer);
            scrSetMessage("use sector %s slope for %s", (ctrl) ? "floor" : "ceiling", buffer);
        }
        else if (ProcessSprites(sprSlopeChg, nStep, BMODE, buffer) > 1)
            scrSetMessage("adjust slope for %s by %d", buffer, nStep);
        else
            scrSetMessage("%s slope: %d", buffer, spriteGetSlope(searchwall));
        
        return PROC_OKUB;
    }
    
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_BACKSLASH(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect;

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];

        if (pSect->alignto)
        {
            scrSetMessage("Sector must be non auto-aligned!");
            return PROC_FAILB;
        }

        ProcessSectors((IS_FLOOR) ? floorSlopeSet : ceilSlopeSet, searchstat, 0, BMODE, buffer);
        scrSetMessage("reset slope for %s", buffer);
        return PROC_OKUB;
    }

    if (IsHoverSprite())
    {
        ProcessSprites(sprSlopeSet, 0, BMODE, buffer);
        scrSetMessage("reset slope for %s", buffer);
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PLUS(char key, char ctrl, char shift, char alt)
{
    XSECTOR* pXSect; HSECTORFUNC* pFunc;
    int nStep = (key == KEY_MINUS) ? 1 : -1, nXSect, i;
    strcpy(buffer, (nStep < 0) ? "more" : "less");

    if (keystatus[KEY_D])
    {
        nStep = mulscale10((key == KEY_MINUS) ? 16 : -16, numpalookups<<5);

        #if 0
        if (gModernMap)
        {
            if (IsHoverSector() && isSkySector(searchsector, searchstat))
            {
                parallaxvisibility = ClipRange(parallaxvisibility + nStep, 0, kMaxVisibility);
                scrSetMessage("Global sky visibility %d (%s)", parallaxvisibility, buffer);
                return PROC_OKUB;
            }
        }
        #endif

        visibility = ClipRange(visibility + nStep, 0, kMaxVisibility);
        scrSetMessage("Global visibility %d (%s)", visibility, buffer);
        return PROC_OKUB;
    }

    if (IsHoverWall())
        TranslateWallToSector();

    if (searchsector >= 0)
    {
        if (ctrl && alt)
        {
            // iterate sector visibility (higher numbers are less)
            if (ProcessSectors(sectVisibilityChg, -1, nStep, BMODE, buffer2) > 1)
                scrSetMessage("%s are %s visible", buffer2, buffer);
            else
                scrSetMessage("%s visibility: %d", buffer2, sector[searchsector].visibility);
            
            return PROC_OKUB;
        }
        
        if (ctrl)           pFunc = sectFXAmplitudeChg;     // iterate lighting effect amplitude
        else if (alt)       pFunc = sectFXFreqChg;          // iterate lighting effect phase
        else if (shift)     pFunc = sectFXPhaseChg;         // iterate lighting effect frequency
        else                return PROC_FAILB;

        ProcessSectors(pFunc, -1, nStep, BMODE, buffer2);
        ProcessSectors(sectFXSetShadeFlags, -1, 1, BMODE, buffer2);
        CleanUp();
        
        if ((pXSect = GetXSect(&sector[searchsector])) != NULL)
        {
            if (ctrl)           scrSetMessage("Amplitude: %d", pXSect->amplitude);
            else if (shift)     scrSetMessage("Phase: %d", pXSect->shadePhase);
            else if (alt)       scrSetMessage("Frequency: %d", pXSect->shadeFreq);
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_C(char key, char ctrl, char shift, char alt)
{
    //Shift+C paste shade matching target to clipboard
    //Alt+C   paste tiles matching target to clipboard

    sectortype* pSect; walltype* pWall; spritetype* pSpr;
    int i, j;

    if (shift)
    {
        if (IsHoverWall())
        {
            pWall = &wall[searchwall];
            j = (IS_WALL) ? pWall->picnum : pWall->overpicnum;

            for (i = 0; i < numwalls; i++)
            {
                if (((IS_WALL)
				        ? wall[i].picnum : wall[i].overpicnum) != j)
                            continue;
                wall[i].shade = tempshade;
            }

            return PROC_OKUB;
        }

        if (IsHoverSector())
        {
           pSect = &sector[searchsector];
           j = (IS_FLOOR) ? pSect->floorpicnum : pSect->ceilingpicnum;

            for (i = 0; i < numsectors; i++)
            {
                if (IS_FLOOR)
                {
                    if (sector[i].floorpicnum != j)
					    continue;
                    sector[i].floorshade = tempshade;
                }
                else
                {
                    if (sector[i].ceilingpicnum != j)
					    continue;
                    sector[i].ceilingshade = tempshade;
                }
            }

            return PROC_OKUB;
        }

        if (IsHoverSprite())
        {
            j = sprite[searchwall].picnum;

            for (i = 0; i < kMaxSprites; i++)
            {
                pSpr = &sprite[i];
                if (pSpr->statnum >= kMaxStatus
                    || pSpr->picnum != j)
                        continue;

                sprite[i].shade = tempshade;
            }

            return PROC_OKUB;
        }
    }

    if (!alt)
        return PROC_FAILB;

    if (IsHoverWall())
    {
        pWall = &wall[searchwall];
        j = (IS_WALL) ? pWall->picnum : pWall->overpicnum;

        if (wallInHglt(searchwall))
        {
            for (i = 0; i < highlightcnt; i++)
            {
                if ((highlight[i] & 0xC000) != 0)
                    continue;

                pWall = &wall[highlight[i]];
                if (IS_WALL)
                {
                    if (pWall->picnum != j)                 continue;
                    else if (pWall->picnum != temppicnum)   pWall->picnum = temppicnum;
                    else if (pWall->pal != temppal)         pWall->pal = temppal;
                }
                else if (pWall->overpicnum == j)
                {
                    pWall->overpicnum = temppicnum;
                }
            }
        }
        else
        {
            for (i = 0; i < numwalls; i++)
            {
                pWall = &wall[i];
                if (IS_WALL)
                {
                    if (pWall->picnum != j)                 continue;
                    else if (pWall->picnum != temppicnum)   pWall->picnum = temppicnum;
                    else if (pWall->pal != temppal)         pWall->pal = temppal;
                }
                else if (pWall->overpicnum == j)
                {
                    pWall->overpicnum = temppicnum;
                }
            }
        }

        return PROC_OKUB;
    }

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];
        j = (IS_FLOOR) ? pSect->floorpicnum : pSect->ceilingpicnum;

        if (sectInHglt(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                pSect = &sector[highlightsector[i]];
                if (IS_FLOOR)
                {
                    if (pSect->floorpicnum != j)                    continue;
                    else if (pSect->floorpicnum != temppicnum)      pSect->floorpicnum = temppicnum;
                    else if (pSect->floorpal != temppal)            pSect->floorpal = temppal;
                }
                else
                {
                    if (pSect->ceilingpicnum != j)                  continue;
                    else if (pSect->ceilingpicnum != temppicnum)    pSect->ceilingpicnum = temppicnum;
                    else if (pSect->ceilingpal != temppal)          pSect->ceilingpal = temppal;
                }
            }
        }
        else
        {
            for (i = 0; i < numsectors; i++)
            {
                pSect = &sector[i];
                if (IS_FLOOR)
                {
                    if (pSect->floorpicnum != j)                    continue;
                    else if (pSect->floorpicnum != temppicnum)      pSect->floorpicnum = temppicnum;
                    else if (pSect->floorpal != temppal)            pSect->floorpal = temppal;
                }
                else
                {
                    if (pSect->ceilingpicnum != j)                  continue;
                    else if (pSect->ceilingpicnum != temppicnum)    pSect->ceilingpicnum = temppicnum;
                    else if (pSect->ceilingpal != temppal)          pSect->ceilingpal = temppal;
                }
            }
        }

        return PROC_OKUB;
    }

    if (IsHoverSprite())
    {
        j = sprite[searchwall].picnum;
        if (sprInHglt(searchwall))
        {
            for (i = 0; i < highlightcnt; i++)
            {
                if ((highlight[i] & 0xC000) == 0)
                    continue;

                pSpr = &sprite[highlight[i] & 0x3FFF];
                if (pSpr->statnum >= kMaxStatus
                    || pSpr->picnum != j)
                        continue;

                pSpr->picnum = temppicnum;
                pSpr->type = temptype;
            }
        }
        else
        {
            for (i = 0; i < kMaxSprites; i++)
            {
                pSpr = &sprite[i];
                if (pSpr->statnum >= kMaxStatus
                    || pSpr->picnum != j)
                        continue;

                pSpr->picnum = temppicnum;
                pSpr->type = temptype;
            }
        }

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect; walltype* pWall; spritetype* pSpr;
    int i;

    if (alt)
    {
        switch (searchstat)
        {
            case OBJ_WALL:
            case OBJ_MASKED:
                searchsector = sectorofwall(searchwall);
                sectCstatRem(searchsector, kSectFlipMask, OBJ_CEILING);
                sectCstatAdd(searchsector, kSectRelAlign, OBJ_CEILING);
                setFirstWall(searchsector, searchwall);
                return PROC_OKUB;
            case OBJ_CEILING:
            case OBJ_FLOOR:
                sectCstatRem(searchsector, kSectFlipMask, searchstat);
                sectCstatAdd(searchsector, kSectRelAlign, searchstat);
                setFirstWall(searchsector, sector[searchsector].wallptr + 1);
                return PROC_OKUB;
        }

        return PROC_FAILB;
    }

    if (IsHoverWall())
    {
        pWall = &wall[searchwall];

        switch(pWall->cstat & kWallFlipMask)
        {
            case 0:                     i = kWallFlipX;                 break;
            case kWallFlipX:            i = kWallFlipX|kWallFlipY;      break;
            case kWallFlipX|kWallFlipY: i = kWallFlipY;                 break;
            case kWallFlipY:            i = 0;                          break;
        }
        
        ProcessWalls(wallCstatRem, kWallFlipMask, !shift, BMODE, NULL);
        ProcessWalls(wallCstatAdd, i, !shift, BMODE, buffer);

        if (pWall->cstat & kWallFlipX)
            strcat(buffer," x-flipped");

        if (pWall->cstat & kWallFlipY)
        {
            if (pWall->cstat & kWallFlipX)
                strcat(buffer," and");

            strcat(buffer," y-flipped");
        }

        scrSetMessage(buffer);
        return PROC_OKUB;
    }

    if (IsHoverSector())
    {
        if (!isSkySector(searchsector, searchstat))
        {
            pSect = &sector[searchsector];
            i = (IS_FLOOR)
                ? pSect->floorstat : pSect->ceilingstat;

            switch (i & kSectFlipMask)
            {
                case 0x00: i = 0x10; break;
                case 0x10: i = 0x30; break;
                case 0x30: i = 0x20; break;
                case 0x20: i = 0x04; break;
                case 0x04: i = 0x14; break;
                case 0x14: i = 0x34; break;
                case 0x34: i = 0x24; break;
                case 0x24: i = 0x00; break;
            }

            ProcessSectors((IS_FLOOR) ? floorStatRem : ceilStatRem, searchstat, kSectFlipMask, BMODE);
            ProcessSectors((IS_FLOOR) ? floorStatAdd : ceilStatAdd, searchstat, i, BMODE);
            return PROC_OKUB;
        }
    }

    if (IsHoverSprite())
    {
        pSpr = &sprite[searchwall];
        
        switch(pSpr->cstat & kSprFlipMask)
        {
            case 0x00:                  i = kSprFlipX;              break;
            case kSprFlipX:             i = kSprFlipX|kSprFlipY;    break;
            case kSprFlipX|kSprFlipY:   i = kSprFlipY;              break;
            default:                    i = 0x00;                   break;
        }
        
        ProcessSprites(sprFlipSet, i, BMODE, buffer);
        
        if (pSpr->cstat & kSprFlipX)
            strcat(buffer," x-flipped");

        if (pSpr->cstat & kSprFlipY)
        {
            if (pSpr->cstat & kSprFlipX)
                strcat(buffer," and");

            strcat(buffer," y-flipped");
        }
        
        scrSetMessage(buffer);
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_G(char key, char ctrl, char shift, char alt)
{
    if (shift)
    {
        if (!ctrl)
        {
            grid = IncRotate(grid, 11);
            scrSetMessage("Grid size: %d", grid);
        }
        else
        {
            gridlock = !gridlock;
            scrSetMessage("Gridlock is %s", onOff(gridlock));
        }

        return PROC_OKB;
    }


    if (alt)
    {
        // change global palette
        gMisc.palette = getClosestId(gMisc.palette, kPalMax - 1, "PAL", 1);
        scrSetMessage("Screen palette #%d", gMisc.palette);
        scrSetPalette(gMisc.palette);
        return PROC_OKB;
    }

    if (gListGrd.Exists(searchstat, GetHoverID()))
    {
        if (gListGrd.Length() > 1)
        {
            grshShadeWalls(ctrl);
            return PROC_OKUB;
        }

        scrSetMessage("Must highlight at least 2 objects!");
        return PROC_FAILB;
    }

    scrSetMessage("Object is not highlighted!");
    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_H(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; walltype* pWall;
    
    switch (searchstat)
    {
        case OBJ_FLOOR:
        case OBJ_CEILING:
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            pWall = &wall[searchwall];
            ProcessWalls((pWall->cstat & kWallHitscan) ? wallCstatRem : wallCstatAdd, kWallHitscan, !shift, BMODE, buffer);
            scrSetMessage("%s %s hitscan sensitive.", buffer, isNot(pWall->cstat & kWallHitscan));
            return PROC_OKUB;
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            ProcessSprites((pSpr->cstat & kSprHitscan) ? sprCstatRem : sprCstatAdd, kSprHitscan, BMODE, buffer);
            scrSetMessage("%s %s hitscan sensitive.", buffer, isNot(pSpr->cstat & kSprHitscan));
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_L(char key, char ctrl, char shift, char alt)
{
    int oCstat, zTop, zBot;
    spritetype* pSpr;

    if (ctrl)
        return (dlgLightBombOptions(&gLightBomb)) ? PROC_OKB : PROC_OK;

    if (IsHoverSector())
    {
        Sky::ToggleFloorShade(searchsector, alt);
        return PROC_OKUB;
    }

    if (IsHoverSprite())
    {
        pSpr = &sprite[searchwall]; oCstat = pSpr->cstat;
        GetSpriteExtents(pSpr, &zTop, &zBot);
        pSpr->cstat &= ~kSprHitscan;

        SetupLightBomb();
        LightBomb(pSpr->x, pSpr->y, zTop, pSpr->sectnum);
        pSpr->cstat = oCstat;
        return PROC_OKUB;
    }

    if (IsHoverWall())
    {
        int hx, hy, hz, s; short hsc, hsp, hwl;
        camHitscan(&hsc, &hwl, &hsp, &hx, &hy, &hz, BLOCK_NONE);
        offsetPos(0, 256, 0, GetWallAngle(searchwall) + kAng90, &hx, &hy, NULL);

        s = sectorofwall(searchwall);
        if (FindSector(hx, hy, hz, &s))
        {
            SetupLightBomb();
            LightBomb(hx, hy, hz, s);
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_O(char key, char ctrl, char shift, char alt)
{
    walltype* pWall; spritetype* pSpr;
    int nHit, nAng;

    if (IsHoverWall())
    {
        // O (top/bottom orientation - for doors)

        pWall = &wall[searchwall];
        ProcessWalls((pWall->cstat & kWallOrgBottom) ? wallCstatRem : wallCstatAdd, kWallOrgBottom, false, BMODE, buffer);
        scrSetMessage("Texture pegged at %s for %s", (pWall->cstat & kWallOrgBottom) ? "bottom" : "top", buffer);
        return PROC_OKUB;
    }

    if (IsHoverSprite())
    {
        // O (ornament onto wall)
        pSpr = &sprite[searchwall];

        if (ctrl)
        {
            if (pSpr->z > sector[pSpr->sectnum].floorz)
            {
                scrSetMessage("%s[%d].z is below floor", GetHoverName(), searchwall);
                return PROC_FAILB;
            }

            if (pSpr->z < sector[pSpr->sectnum].ceilingz)
            {
                scrSetMessage("%s[%d].z is above ceiling", GetHoverName(), searchwall);
                return PROC_FAILB;
            }

            nAng = ang;
            if (alt)
                nAng = pSpr->ang + kAng180;

            switch(HitScan(pSpr, pSpr->z, Cos(nAng)>>16, Sin(nAng)>>16, 0, BLOCK_NONE, 0))
            {
                case OBJ_WALL:
                case OBJ_MASKED:
                    pSpr->x = gHitInfo.hitx;
                    pSpr->y = gHitInfo.hity;
                    pSpr->z = gHitInfo.hitz;

                    pSpr->ang = (short)((GetWallAngle(gHitInfo.hitwall) + kAng90) & kAngMask);
                    doWallCorrection(gHitInfo.hitwall, &pSpr->x, &pSpr->y);
                    ChangeSpriteSect(pSpr->index, gHitInfo.hitsect);

                    scrSetMessage("%s ornamented onto wall %d\n", GetHoverName(), gHitInfo.hitwall);
                    return PROC_OKUB;
            }
        }
        else if (alt)
        {
            ProcessSprites((pSpr->cstat & kSprOrigin) ? sprCstatRem : sprCstatAdd, kSprOrigin, BMODE, buffer);
            scrSetMessage("origin align is %s for %s", onOff(pSpr->cstat & kSprOrigin), buffer);
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_P(char key, char ctrl, char shift, char alt)
{
    char title[64], nPlu;
    int nID, nPic, nShade;
    int i;

    nID = GetHoverID();

    if (IsHoverSector())
    {
        if (ctrl && shift)
        {
            if (isSkySector(searchsector, searchstat))
            {
                Sky::Disable(searchsector, searchstat, alt);
                return PROC_OKUB;
            }

            return PROC_FAILB;
        }

        if (alt)
        {
            sectCstatToggle(searchsector, kSectParallax, searchstat);
            if (isSkySector(searchsector, searchstat)) Sky::MakeSimilar(searchsector, searchstat, 0);
            else if (searchstat == OBJ_CEILING) sectCstatRem(searchsector, kSectShadeFloor, searchstat);
            scrSetMessage("%s[%d] %s parallaxed", GetHoverName(), searchsector, isNot(isSkySector(searchsector, searchstat)));
            return PROC_OKUB;
        }
    }

    nPlu    = getPluOf(searchstat,      nID);
    nPic    = getPicOf(searchstat,      nID);
    nShade  = getShadeOf(searchstat,    nID);
    sprintf(title, "%s #%d palookup", GetHoverName(), nID);

    if (!shift)
    {
        i = nPlu;
        if (IsHoverSprite())
            nShade = viewSpriteShade(nShade, nPic, sprite[searchwall].sectnum);

        if ((nPlu = pluPick(nPic, nShade, nPlu, title)) == i)
            return PROC_FAILB;
    }
    else
    {
        nPlu = nextEffectivePlu(nPic, 0, nPlu, (shift & 0x01));
    }

    if (IsHoverSector())
    {
        if (isSkySector(searchsector, searchstat))
        {
            Sky::SetPal(searchsector, searchstat, nPlu, ctrl);
            return PROC_OKUB;
        }
    }

    i = isEffectivePLU(nPic, palookup[nPlu]);
    buffer[0] = '\0';
    if (nPlu > 1)
        sprintf(buffer, "(%d%% efficiency)", i);
    
    switch(searchstat)
    {
        case OBJ_CEILING:
            i = ProcessSectors(ceilPalSet, searchstat, nPlu, BMODE, buffer2);
            break;
        case OBJ_FLOOR:
            i = ProcessSectors(floorPalSet, searchstat, nPlu, BMODE, buffer2);
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            i = ProcessWalls(wallPalSet, nPlu, !shift, BMODE, buffer2);
            break;
        case OBJ_SPRITE:
            i = ProcessSprites(sprPalSet, nPlu, BMODE, buffer2);
            break;
    }
    
    scrSetMessage("%s palookup #%d %s", buffer2, nPlu, (i == 1) ? buffer : "");
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_S(char key, char ctrl, char shift, char alt)
{
    short hitsect = -1, hitwall = -1, hitsprite = -1;
    int i, x, y, z;
    int nPic;

    spritetype* pSpr;

    if (IsHoverSprite()
        && (sprite[searchwall].cstat & kSprRelMask) == kSprFace)
            return PROC_FAILB;

    if (camHitscan(&hitsect, &hitwall, &hitsprite, &x, &y, &z, BLOCK_MOVE | BLOCK_HITSCAN) < 0)
        return PROC_FAILB;

    i = -1;
    if (!alt)
    {
        nPic = OBJ_SPRITE;
        if (somethingintab != OBJ_SPRITE)
        {
            switch(searchstat)
            {
                case OBJ_SPRITE:
                    switch (sprite[searchwall].cstat & kSprRelMask)
                    {
                        case kSprFloor:
                        case kSprSloped:
                        case kSprWall:
                            nPic = OBJ_FLATSPRITE;
                            break;
                    }
                    break;
                case OBJ_WALL:
                    nPic = OBJ_FLATSPRITE;
                    break;
            }

            if ((nPic = tilePick(-1, -1, nPic)) < 0)
                return PROC_FAILB;
        }

        if ((i = InsertSprite(hitsect, kStatDecoration)) >= 0)
        {
            pSpr = &sprite[i];
            if (somethingintab == OBJ_SPRITE)
            {
                pSpr->picnum    = temppicnum;
                pSpr->shade     = tempshade;
                pSpr->pal       = temppal;
                pSpr->xrepeat   = tempxrepeat;
                pSpr->yrepeat   = tempyrepeat;
                pSpr->xoffset   = (char)tempxoffset;
                pSpr->yoffset   = (char)tempyoffset;
                pSpr->cstat     = (short)tempcstat;
            }
            else
            {
                pSpr->picnum = (short)nPic;
                pSpr->shade  = -8;
            }
        }
    }
    else
    {
        i = InsertGameObject(searchstat, hitsect, x, y, z, ang);
    }

    if (i < 0)
        return PROC_FAILB;


    pSpr    = &sprite[i];
    pSpr->x = x;
    pSpr->y = y;
    pSpr->z = z;

    AutoAdjustSprites();

    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            doWallCorrection(searchwall2, &pSpr->x, &pSpr->y);
            pSpr->ang = (short)((GetWallAngle(searchwall2) + kAng90) & kAngMask);
            if ((pSpr->cstat & kSprRelMask) != kSprWall)
            {
                pSpr->cstat &= ~kSprRelMask;
                pSpr->cstat |= kSprWall;
            }
            pSpr->cstat |= kSprOneSided;
            break;
        case OBJ_SPRITE:
            switch (sprite[searchwall].cstat & kSprRelMask)
            {
                case kSprSloped:
                case kSprFloor:
                    pSpr->z = sprite[searchwall].z;
                    clampSpriteZ(&sprite[i], sprite[searchwall].z,
                            (posz > sprite[searchwall].z) ? 0x01 : 0x02);

                    if (((pSpr->cstat & kSprRelMask) == kSprWall)
                        && (pSpr->cstat & kSprOneSided))
                                pSpr->ang = (short)((ang + kAng180) & kAngMask);
                    break;
                case kSprWall:
                    pSpr->ang = sprite[searchwall].ang;
                    pSpr->cstat |= kSprWall;
                    break;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            clampSprite(pSpr);
            helperDoGridCorrection(&pSpr->x, &pSpr->y);

            if (((pSpr->cstat & kSprRelMask) == kSprWall)
                && (pSpr->cstat & kSprOneSided))
                    pSpr->ang = (short)((ang + kAng180) & kAngMask);
            break;
    }

    scrSetMessage("Sprite inserted.");
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_T(char key, char ctrl, char shift, char alt)
{
    spritetype* pSpr; walltype* pWall; sectortype* pSect;
    int i = 0, j;

    switch (searchstat)
    {
        case OBJ_MASKED:
            pWall = &wall[searchwall];
            if (pWall->cstat & kWallTransluc)  i = 2;
            if (pWall->cstat & kWallTranslucR) i = 1;
            switch (i = IncRotate(i, 3))
            {
                case 0:
                    ProcessWalls(wallCstatRem, kWallTransluc2, !shift, BMODE, buffer);
                    break;
                case 1:
                    ProcessWalls(wallCstatAdd, kWallTransluc2, !shift, BMODE, buffer);
                    break;
                case 2:
                    ProcessWalls(wallCstatRem, kWallTransluc2, !shift, BMODE, buffer);
                    ProcessWalls(wallCstatAdd, kWallTransluc, !shift, BMODE, buffer);
                    break;
            }
            scrSetMessage("%s %s tanslucent", buffer, gTranslucLevNames[i]);
            break;
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            if (pSpr->cstat & kSprTransluc1) i = 2;
            if (pSpr->cstat & kSprTranslucR) i = 1;
            switch (i = IncRotate(i, 3))
            {
                case 0:
                    ProcessSprites(sprCstatRem, kSprTransluc2, BMODE, buffer);
                    break;
                case 1:
                    ProcessSprites(sprCstatAdd, kSprTransluc2, BMODE, buffer);
                    break;
                case 2:
                    ProcessSprites(sprCstatRem, kSprTransluc2, BMODE, buffer);
                    ProcessSprites(sprCstatAdd, kSprTransluc1, BMODE, buffer);
                    break;
            }
            scrSetMessage("%s %s tanslucent", buffer, gTranslucLevNames[i]);
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            pSect = &sector[searchsector]; j = IS_FLOOR;
            switch (sectCstatGet(searchsector, searchstat) & kSectTranslucR)
            {
                case 0: // first set it just masked
                    ProcessSectors(j ? floorStatAdd : ceilStatAdd, searchstat, kSectMasked, BMODE, buffer);
                    scrSetMessage("%s is masked", buffer);
                    break;
                case kSectMasked:
                    ProcessSectors(j ? floorStatAdd : ceilStatAdd, searchstat, kSectTransluc, BMODE, buffer);
                    scrSetMessage("%s %s tanslucent", buffer, gTranslucLevNames[1]);
                    break;
                case kSectTranslucR:
                    ProcessSectors(j ? floorStatAdd : ceilStatAdd, searchstat, kSectTranslucR, BMODE, buffer);
                    ProcessSectors(j ? floorStatRem : ceilStatRem, searchstat, kSectMasked, BMODE, buffer);
                    scrSetMessage("%s %s tanslucent", buffer, gTranslucLevNames[2]);
                    break;
                case kSectTransluc:
                    ProcessSectors(j ? floorStatRem : ceilStatRem, searchstat, kSectTranslucR, BMODE, buffer);
                    scrSetMessage("%s is not masked nor translucent", buffer);
                    break;
            }
            break;
        default:
            return PROC_FAILB;
    }

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

int nextTile(int nTile, char f)
{
    int oTile = nTile;
    int c = 0;

    while(++c < gMaxTiles)
    {
        if (f & 0x01)
            nTile = IncRotate(nTile, gMaxTiles);
        else
            nTile = DecRotate(nTile, gMaxTiles);

        if (tileLoadTile(nTile))
        {
            if ((f & 0x02) != 0 && tileHasColor(nTile, 255)) continue;
            if ((f & 0x04) != 0 && !POWNICE(nTile)) continue;

            return nTile;
        }
    }

    return oTile;
}

static char edKeyProc3D_KEY_V(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect; walltype* pWall; spritetype* pSpr;
    int i, oPic = -1, nPic = -1;
    char flags = 0x0;


    if (shift & 0x01)
        flags |= 0x01;

    if (alt && gFavTiles.Length() <= 0)
    {
        Alert("There is no favorite tiles added yet.");
        return PROC_FAILB;
    }

    i = (IsHoverSector()) ? searchsector : searchwall;
    sprintf(buffer, "%s #%d picnum", GetHoverName(), i);

    switch (searchstat)
    {
        case OBJ_SPRITE:

            pSpr = &sprite[searchwall];
            oPic = pSpr->picnum;
            if ((pSpr->cstat & kSprRelMask) != kSprFace)
                searchstat = OBJ_FLATSPRITE;

            if (alt)        nPic = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = nextTile(oPic, flags);
            else            nPic = tilePick(oPic, oPic, searchstat, buffer);
            
            if (nPic == oPic) break;
            ProcessSprites(sprPicnumSet, nPic, BMODE, buffer);
            scrSetMessage("%s picnum: %d", buffer, nPic);
            break;

        case OBJ_WALL:

            pWall = &wall[searchwall];
            oPic = pWall->picnum;

            if (alt)        nPic = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = nextTile(oPic, flags|0x06);
            else            nPic = tilePick(oPic, oPic, searchstat, buffer);
            
            if (nPic == oPic) break;
            ProcessWalls(wallPicnumSet, nPic, false, BMODE, buffer);
            scrSetMessage("%s picnum: %d", buffer, nPic);
            break;

        case OBJ_MASKED:

            pWall = &wall[searchwall];
            oPic = pWall->overpicnum;

            if (alt)        nPic = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = nextTile(oPic, flags|0x04);
            else            nPic = tilePick(oPic, oPic, searchstat, buffer);
            
            if (nPic == oPic) break;
            ProcessWalls(wallPicnumMaskSet, nPic, true, BMODE, buffer);
            scrSetMessage("%s mask picnum: %d", buffer, nPic);
            break;

        case OBJ_CEILING:
        case OBJ_FLOOR:

            pSect = &sector[searchsector];
            if (IS_FLOOR)
            {
                oPic = pSect->floorpicnum;
                if (alt)        nPic = favoriteTileSelect(oPic, oPic, searchstat);
                else if (shift) nPic = nextTile(oPic, flags|0x06);
                else            nPic = tilePick(oPic, oPic, searchstat, buffer);
            }
            else
            {
                oPic = pSect->ceilingpicnum;
                if (alt)        nPic = favoriteTileSelect(oPic, oPic, searchstat);
                else if (shift) nPic = nextTile(oPic, flags|0x06);
                else            nPic = tilePick(oPic, oPic, searchstat, buffer);
            }
            
            if (nPic == oPic) break;
            ProcessSectors(IS_FLOOR ? floorPicnumSet : ceilPicnumSet, searchstat, nPic, BMODE, buffer);
            if (!IS_FLOOR)
                ProcessSectors(ceilStatRem, searchstat, kSectShadeFloor, BMODE);
            
            scrSetMessage("%s picnum: %d", buffer, nPic);
            break;
    }

    return (oPic == nPic) ? PROC_OK : PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PADMINUS(char key, char ctrl, char shift, char alt)
{
    typedef void (*PSETSHADEOF)(int nShade, int oType, int oIdx);
    PSETSHADEOF pSetShadeOf = (ctrl) ? setShadeOf : iterShadeOf;
    int nStep = (ctrl) ? 128 : 1;
    int nSect, i, s, e;

    if (key == KEY_PADPLUS)
        nStep = -nStep;

    if (gListGrd.Exists(searchstat, GetHoverID()))
    {
        OBJECT* pFirst = gListGrd.Ptr();
        OBJECT* pDb;

        // check if at least one of highlighted objects gets reached min/max shade
        if (nStep > 0)
        {
            for (pDb = pFirst; pDb->type != OBJ_NONE; pDb++)
            {
                if (getShadeOf(pDb->type, pDb->index) < 127) continue;
                scrSetMessage("One of objects reached max shade!");
                return PROC_FAILB;
            }
        }
        else
        {
            for (pDb = pFirst; pDb->type != OBJ_NONE; pDb++)
            {
                if (getShadeOf(pDb->type, pDb->index) > -128) continue;
                scrSetMessage("One of objects reached min shade!");
                return PROC_FAILB;
            }
        }

        // set shade relatively
        for (pDb = pFirst; pDb->type != OBJ_NONE; pDb++)
            pSetShadeOf(nStep, pDb->type, pDb->index);

        if (nStep > 0) scrSetMessage("Relative shading (shade: +%d) for %d objects", nStep, gListGrd.Length());
        else scrSetMessage("Relative brighting (shade: -%d) for %d objects", nStep, gListGrd.Length());
        return PROC_OKUB;
    }

    if (sectInHglt(searchsector))
    {
        for (i = 0; i < highlightsectorcnt; i++)
        {
            nSect = highlightsector[i];
            pSetShadeOf(nStep, OBJ_CEILING, nSect);
            pSetShadeOf(nStep, OBJ_FLOOR, nSect);

            getSectorWalls(nSect, &s, &e);
            while(s <= e)
                pSetShadeOf(nStep, OBJ_WALL, s), s++;
        }

        return PROC_OKUB;
    }

    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            pSetShadeOf(nStep, searchstat, searchwall);
            scrSetMessage("Shade: %d", wall[searchwall].shade);
            return PROC_OKUB;
        case OBJ_CEILING:
            pSetShadeOf(nStep, searchstat, searchsector);
            scrSetMessage("Shade: %d", sector[searchsector].ceilingshade);
            if (isSkySector(searchsector, searchstat))
                Sky::SetShade(searchsector, searchstat, sector[searchsector].ceilingshade, alt);
            return PROC_OKUB;
        case OBJ_FLOOR:
            pSetShadeOf(nStep, searchstat, searchsector);
            scrSetMessage("Shade: %d", sector[searchsector].floorshade);
            if (isSkySector(searchsector, searchstat))
                Sky::SetShade(searchsector, searchstat, sector[searchsector].floorshade, alt);
            return PROC_OKUB;
        case OBJ_SPRITE:
            if (ProcessSprites((ctrl) ? sprShadeSet : sprShadeChg, nStep, BMODE, buffer) > 1)
                scrSetMessage("change shade by %d for %s", nStep, buffer);
            else
                scrSetMessage("Shade: %d", sprite[searchwall].shade);
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PAD0(char key, char ctrl, char shift, char alt)
{
    char pal = (keystatus[KEY_PADPERIOD] != 0);
    int nSect, i, s, e;

    if (sectInHglt(searchsector))
    {
        i = highlightsectorcnt;
        while(--i >= 0)
        {
            nSect = highlightsector[i];
            (pal) ? sector[nSect].ceilingpal = 0 : sector[nSect].ceilingshade = 0;
            (pal) ? sector[nSect].floorpal   = 0 : sector[nSect].floorshade   = 0;

            getSectorWalls(nSect, &s, &e);
            while(s <= e)
                (pal) ? wall[s].pal = 0 : wall[s].shade = 0, s++;
        }
    }
    else
    {
        switch (searchstat)
        {
            case OBJ_WALL:
            case OBJ_MASKED:
                ProcessWalls((pal) ? wallPalSet : wallShadeSet, 0, false, BMODE, buffer);
                break;
            case OBJ_FLOOR:
                ProcessSectors((pal) ? floorPalSet : floorShadeSet, searchstat, 0, BMODE, buffer);
                break;
            case OBJ_CEILING:
                ProcessSectors((pal) ? ceilPalSet : ceilShadeSet, searchstat, 0, BMODE, buffer);
            case OBJ_SPRITE:
                ProcessSprites((pal) ? sprPalSet : sprShadeSet, 0, BMODE, buffer);
                break;
            default:
                return PROC_FAILB;
        }
        
        scrSetMessage("%s %s reset.", buffer, (pal) ? "palette" : "shade");
    }
    
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F2(char key, char ctrl, char shift, char alt)
{
    char* errMsg;
    int i, nID;


    if (alt)
    {
        // reverse door position
        if (IsHoverWall())
            TranslateWallToSector();

        if (IsHoverSector())
        {
            if ((i = reverseSectorPosition(searchsector)) < 0)
            {
                if ((errMsg = retnCodeCheck(i, gReverseSectorErrors)) != NULL)
                    Alert(errMsg);

                return PROC_FAILB;
            }

            return PROC_OKUB;
        }

        return PROC_FAILB;
    }

    // toggle xstructure state
    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_MASKED:
            if ((nID = wall[searchwall].extra) > 0)
            {
                ProcessWalls(wallStateSet, !xwall[nID].state, false, BMODE, buffer);
                scrSetMessage("%s state is %s.", buffer, onOff(xwall[nID].state));
                return PROC_OKUB;
            }
            break;
        case OBJ_CEILING:
        case OBJ_FLOOR:
            if ((nID = sector[searchsector].extra) > 0)
            {
                ProcessSectors(sectStateSet, -1, !xsector[nID].state, BMODE, buffer);
                scrSetMessage("%s state is %s.", buffer, onOff(xsector[nID].state));
                return PROC_OKUB;
            }
            break;
        case OBJ_SPRITE:
            if ((nID = sprite[searchwall].extra) > 0)
            {
                ProcessSprites(sprStateSet, !xsprite[nID].state, BMODE, buffer);
                scrSetMessage("%s state is %s.", buffer, onOff(xsprite[nID].state));
                return PROC_OKUB;
            }
            break;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F3(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect;
    int nVal;

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector())
    {
        nVal = (key == KEY_F4);
        
        if (alt)
            nVal |= 0x02;
        
        pSect = &sector[searchsector];
        sprintf(buffer, "%s %s", (nVal & 0x02) ? "Capture" : "Show", onOff(nVal & 0x01));
        strupr(buffer);
        
        switch(ProcessSectors(sectShowOrSetupZMotion, -1, nVal, BMODE, buffer2))
        {
            case 1:
                scrSetMessage("%s %s floorZ=%d, ceilingZ=%d", buffer, buffer2, pSect->floorz, pSect->ceilingz);
                return PROC_OKUB;
            case 0:
                if (pSect->type == kSectorZMotionSprite && (nVal & 0x02) == 0)
                {
                    scrSetMessage("Preview unavailable for %s!", gSectorNames[pSect->type]);
                    break;
                }
                scrSetMessage("Sector type #%d doesn't have z-motion!", pSect->type);
                break;
            default:
                scrSetMessage("%s for %s", buffer, buffer2);
                return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F9(char key, char ctrl, char shift, char alt)
{
    return F9Menu() ? PROC_OKUB : PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_2(char key, char ctrl, char shift, char alt)
{
    // 2 (bottom wall swapping)

    if (IsHoverSprite()
        || searchwall < 0 || wall[searchwall].nextwall < 0)
                return PROC_FAILB;

    searchwall = searchwall2; walltype* pWall = &wall[searchwall];
    ProcessWalls((pWall->cstat & kWallSwap) ? wallCstatRem : wallCstatAdd, kWallSwap, false, false, buffer);
    scrSetMessage("%s %s bottom swapped", buffer, isNot(pWall->cstat & kWallSwap));
    return PROC_OKUB;
}

////////////////////////////////////////////////////
/***********************************************/
/** 2D MODE ONLY MAP EDITING INPUT FUNCTIONS  **/
/***********************************************/
//////////////////

static char edKeyProc2D_KEY_BACKSPACE(char key, char ctrl, char shift, char alt)
{
    if (pGLBuild)
    {
        pGLBuild->RemAutoPoints();
        pGLBuild->RemPoint();
        if (pGLBuild->NumPoints() <= 0)
            DELETE_AND_NULL(pGLBuild);

        return PROC_OKB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_SPACE(char key, char ctrl, char shift, char alt)
{
    int x = mousxplc, y = mousyplc;
    int nStat; char* p;

    helperDoGridCorrection(&x, &y);
    sectorToolDisableAll(alt);

    if (key == KEY_SPACE)
    {
        if (alt)
        {
            if (sectorToolDlgLauncher())
            {
                gJoinSector = -1;
                return PROC_OKB;
            }

            return PROC_FAILB;
        }

        if (shift)
        {
            if (pointhighlight >= 0 && (pointhighlight & 0xc000) == 0)
                getWallCoords(pointhighlight, &x, &y);
        }

        if (!pGLBuild)
        {
            pGLBuild = new LOOPBUILD();

            pGLBuild->Setup(x, y);
            if ((nStat = pGLBuild->StatusGet()) == 0)
            {
                gJoinSector = -1;
                pGLBuild->AddPoint(x, y);
                return PROC_OKB;
            }

            if ((p = retnCodeCheck(nStat, gLoopBuildErrors)) != NULL)
                gMapedHud.SetMsgImp(128, "%s", p);

            DELETE_AND_NULL(pGLBuild);
            return PROC_FAILB;
        }
    }

    if (key == KEY_C)
    {
        if (sectorToolEnable(kSectToolCurveWall, 0))
            return PROC_OKB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_INSERT(char key, char ctrl, char shift, char alt)
{
    int x = mousxplc, y = mousyplc;
    int i, j;

    if (!pGLBuild)
    {
        i = highlightsectorcnt;
        while(--i >= 0)
        {
            if (!inside(x, y, highlightsector[i])) continue;
            ProcessSectors(sectClone, -1, 0, true, buffer);
            scrSetMessage("%s duplicated and stamped.", buffer);
            hgltSectAttach(true);
            return PROC_OKUB;
        }

        if (IsHoverSprite())
        {
            ProcessSprites(sprClone, 0, true, buffer);
            scrSetMessage("%s duplicated and stamped.", buffer);
            if (pointdrag >= 0 && (pointdrag & 0xC000) != 0)
            {
                i = j = pointdrag & 0x3FFF;
                if (!sprInHglt(i))
                {
                    x = sprite[i].x;
                    y = sprite[i].y;

                    while((i = nextSpriteAt(x, y, i)) != j)
                    {
                        if (sprInHglt(i))
                        {
                            ChangeSpriteSect(sprite[i].index, sprite[i].sectnum);
                            pointhighlight = i | 0x4000;
                            break;
                        }
                    }
                }
            }

            return PROC_OKUB;
        }
    }

    if (linehighlight >= 0)
    {
        getclosestpointonwall(x, y, linehighlight, &x, &y);
        if (!shift)
            helperDoGridCorrection(&x, &y);

        if (findWallAtPos(x, y) < 0)
        {
            insertPoint(linehighlight, x, y);
            scrSetMessage("New point inserted at X:%d Y:%d", x, y);
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_F8(char key, char ctrl, char shift, char alt)
{
    int i = (alt) ? 0 : 1;
    int c = 0;
    
    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_SPRITE:
            c = CXTracker::Track(searchstat, searchwall, i);
            break;
        case OBJ_FLOOR:
            c = CXTracker::Track(OBJ_SECTOR, sectorhighlight, i);
            break;
    }
    
    return (c) ? PROC_OKB : PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_F7(char key, char ctrl, char shift, char alt)
{
    if (sectorhighlight < 0)
        return PROC_FAILB;

    searchstat = OBJ_FLOOR;
    return edKeyProc2D_KEY_F8(key, ctrl, shift, alt);
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_F(char key, char ctrl, char shift, char alt)
{
    int nWall;
    if (pointhighlight >= 0 && (pointhighlight & 0xc000) == 0)  nWall = pointhighlight;
    else if (linehighlight >= 0)                                nWall = linehighlight;
    else                                                        nWall = -1;

    if (nWall < 0)
        return PROC_FAILB;

    if (ctrl)
    {
        int ls, le;
        loopGetWalls(nWall, &ls, &le);
        flipwalls(ls, ls+le-ls);

        loopDetach(ls);
        loopAttach(ls);

        scrSetMessage("%d walls were turned inside out.", le-ls+1);
        return PROC_OKUB;
    }

    if (hgltWallCount())
        return PROC_FAILB;

    setFirstWall(sectorofwall(nWall), nWall);
    scrSetMessage("The wall %d now sector's %d first wall.", nWall, sectorofwall(nWall));
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_H(char key, char ctrl, char shift, char alt)
{
    int i;

    if (IS_EMPTY)
        return PROC_FAILB;

    if (searchstat != OBJ_FLOOR) i = searchwall;
    else i = searchsector, searchstat = OBJ_SECTOR;
    sprintf(buffer, "%s #%d %s: ", GetHoverName(), i, (key == KEY_T) ? "lo-tag" : "hi-tag");

    switch (searchstat)
    {
        case OBJ_WALL:
            if (key == KEY_H)
            {
                if (ctrl)
                    wall[i].hitag = ClipRange(GetNumberBox(buffer, wall[i].hitag, wall[i].hitag), 0, 32767);
                else
                {
                    walltype* pWall = &wall[searchwall];
                    ProcessWalls((pWall->cstat & kWallHitscan) ? wallCstatRem : wallCstatAdd, kWallHitscan, !shift, BMODE, buffer);
                    scrSetMessage("%s %s hitscan sensitive.", buffer, isNot(pWall->cstat & kWallHitscan));
                }
            }
            else wall[i].type = ClipRange(GetNumberBox(buffer, wall[i].type, wall[i].type), 0, 32767);
            return PROC_OKUB;
        case OBJ_SPRITE:
            if (key == KEY_H)
            {
                if (ctrl) sprite[i].flags = ClipRange(GetNumberBox(buffer, sprite[i].flags, sprite[i].flags), 0, 32767);
                else
                {
                    spritetype* pSpr = &sprite[i];
                    ProcessSprites((pSpr->cstat & kSprHitscan) ? sprCstatRem : sprCstatAdd, kSprHitscan, BMODE, buffer);
                    scrSetMessage("%s %s hitscan sensitive.", buffer, isNot(pSpr->cstat & kSprHitscan));
                }
            }
            else sprite[i].type = ClipRange(GetNumberBox(buffer, sprite[i].type, sprite[i].type), 0, 32767);
            return PROC_OKUB;
        case OBJ_SECTOR:
            if (key == KEY_H)
            {
                if (ctrl)
                    sector[i].hitag = ClipRange(GetNumberBox(buffer, sector[i].hitag, sector[i].hitag), 0, 32767);
            }
            else sector[i].type = ClipRange(GetNumberBox(buffer, sector[i].type, sector[i].type), 0, 32767);
            return PROC_OKUB;
    }

    return PROC_FAILB;

}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_T(char key, char ctrl, char shift, char alt)
{
    if (ctrl)
    {
        gScreen2D.prefs.showTags = IncRotate(gScreen2D.prefs.showTags, kCaptionStyleMax);
        scrSetMessage("Show tags: %s", gCaptionStyleNames[gScreen2D.prefs.showTags]);
        return (gScreen2D.prefs.showTags) ? PROC_OKB : PROC_FAILB;
    }

    return edKeyProc2D_KEY_H(key, ctrl, shift, alt);
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_G(char key, char ctrl, char shift, char alt)
{
    int nGrid, i;

    if (ctrl && alt)
    {
        if ((i = hgltWallCount()) > 0)
        {
            nGrid = grid;
            sprintf(buffer, "Snap %d wall points to grid... (%d - %d)", i, 1, kMaxGrids);
            if ((nGrid = GetNumberBox(buffer, nGrid, 0)) > 0)
            {
                ProcessWalls(wallGridSnap, nGrid, true, true, NULL);
                return PROC_OKUB;
            }

            return PROC_FAIL;
        }

        return PROC_FAILB;
    }

    if (ctrl) gAutoGrid.enabled^=1;
    else if (alt && grid > 0) grid = 0;
    else grid = ClipLow(IncRotate(grid, (shift) ? kMaxGrids : 7), 1);

    scrSetMessage("Grid size: %d (Autogrid is %s)", grid, onOff(gAutoGrid.enabled));
    return (grid) ? PROC_OKB : PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_L(char key, char ctrl, char shift, char alt)
{
    gridlock = !gridlock;
    scrSetMessage("Grid locking %s", onOff(gridlock));
    return (gridlock) ? PROC_OKB : PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_S(char key, char ctrl, char shift, char alt)
{
    int flags, x, y, dx, dy, dz;
    int nSect; short nSect2;
    int nAng, i, j, k;

    spritetype *pSpr, *pBufSpr;
    XSECTOR* pXSect;

    if (alt)
    {
        if(highlightsectorcnt > 0 && hgltSectInsideBox(mousxplc, mousyplc))
        {
            flags = 0x0;
            if (!shift)
                flags |= 0x01;

            i = hgltSectAutoRedWall(flags);
            scrSetMessage("%d inner loops created.", i);
            return (i > 0) ? PROC_OKUB : PROC_FAILB;
        }

        if (searchstat != OBJ_WALL)
        {
            if ((searchwall = linehighlight) < 0)
                return PROC_FAILB;
        }

        i = redSectorMake(searchwall);
        if (i == 0)         scrSetMessage("Sector created.");
        else if (i == -1)   scrSetMessage("%d is already red sector!", sectorofwall(searchwall));
        else if (i == -3)   scrSetMessage("Max walls reached!");
        else if (i == -4)   scrSetMessage("Max sectors reached!");
        else                scrSetMessage("Can't make a sector out there.");

        return (i == 0) ? PROC_OKUB : PROC_FAILB;
    }

    x = mousxplc, y = mousyplc;
    gScreen2D.GetPoint(searchx, searchy, &x, &y);
    dx = x, dy = y;

    nSect2 = -1;
    updatesector(dx, dy, &nSect2);
    nSect = nSect2;

    if (nSect < 0)
        return PROC_FAILB;

    helperDoGridCorrection(&dx, &dy);
    dz = getflorzofslope(nSect, dx, dy);

    if (shift)
    {
        if ((i = InsertGameObject(OBJ_FLOOR, nSect, dx, dy, dz, 1536)) >= 0)
        {
            pSpr = &sprite[i];
            scrSetMessage("%s sprite inserted.", gSpriteNames[pSpr->type]);
            pSpr->ang = 1536;

            return PROC_OKUB;
        }

        return PROC_FAILB;
    }

    if ((i = InsertSprite(nSect,0)) >= 0)
    {
        pSpr = &sprite[i];
        pSpr->x = dx, pSpr->y = dy, pSpr->z = dz;
        pSpr->ang = 1536;

        if (somethingintab == OBJ_SPRITE)
        {
            pBufSpr = &sprite[tempidx];

            pSpr->picnum  = cpysprite.picnum;
            pSpr->shade   = cpysprite.shade;
            pSpr->pal     = cpysprite.pal;
            pSpr->xrepeat = cpysprite.xrepeat;
            pSpr->yrepeat = cpysprite.yrepeat;
            pSpr->xoffset = cpysprite.xoffset;
            pSpr->yoffset = cpysprite.yoffset;

            if ((cpysprite.cstat & kSprRelMask) == kSprSloped)
                cpysprite.cstat &= ~kSprSloped;

            pSpr->cstat = cpysprite.cstat;

            if (pBufSpr->type == kMarkerPath)
            {
                sprite[i].type = kMarkerPath;
                ChangeSpriteStat(i, kStatPathMarker);

                pSpr->flags    = pBufSpr->flags;
                pSpr->clipdist = pBufSpr->clipdist;
                pSpr->z        = sprite[tempidx].z; // it's worth probably to use same z

                j = GetXSprite(i); k = pBufSpr->extra;

                // create fully new path
                if (k <= 0 || xsprite[k].reference != tempidx)
                {
                    xsprite[j].data1 = findUnusedPath();
                    xsprite[j].data2 = findUnusedPath();
                }
                else
                {
                    if (!markerIsNode(tempidx, 1))
                    {
                        xsprite[j].data1 = findUnusedPath(); // create new first id
                        xsprite[k].data2 = xsprite[j].data1; // connect previous marker with current
                    }
                    else
                    {
                        xsprite[j].data1 = xsprite[k].data2; // start new branch
                    }

                    xsprite[j].data2    = xsprite[j].data1; // finalize current marker?
                    xsprite[j].busyTime = xsprite[k].busyTime;
                    xsprite[j].wave     = xsprite[k].wave;

                    pXSect = pathMarkerFindSector(&xsprite[j]);

                    if (pXSect == NULL)
                    {
                        short nAng = (short)(getangle(pSpr->x - pBufSpr->x, pSpr->y - pBufSpr->y) & kAngMask);
                        sprite[i].ang = nAng;

                        // change angle of the previous marker if it have less than 2 branches only
                        if (!markerIsNode(tempidx, 2))
                            pBufSpr->ang = nAng;
                    }
                    else
                    {
                        sprite[i].ang = cpysprite.ang;
                    }
                }

                // make a loop?
                if (IsHoverSprite() && sprite[searchwall].type == kMarkerPath)
                {
                    DeleteSprite(i);
                    xsprite[k].data2 = xsprite[sprite[searchwall].extra].data1;
                    if (!Confirm("Finish path drawing now?"))
                    {
                        cpysprite   = sprite[searchwall];
                        cpyxsprite  = xsprite[searchwall];
                        tempidx     = searchwall;
                    }
                    else
                    {
                        char* buttons[] = { "Hold", "Break", "Move back" };
                        if (tempidx == searchwall && !pathMarkerFindSector(&xsprite[k]))
                        {
                            if ((i = showButtons(buttons, LENGTH(buttons), "Finalize path...") - mrUser) >= 0)
                            {
                                xsprite[k].data2 = (i == 0) ? xsprite[k].data1 : -i;
                                somethingintab = OBJ_NONE; // path drawing is finished
                            }
                            else
                            {
                                cpysprite   = sprite[searchwall];
                                cpyxsprite  = xsprite[searchwall];
                                tempidx     = searchwall;
                            }
                        }
                        else
                        {
                            somethingintab = OBJ_NONE; // path drawing is finished
                        }
                    }
                }
                else
                {
                    // update clipboard
                    cpysprite   = sprite[i];
                    cpyxsprite  = xsprite[j];
                    tempidx     = (short)i;
                }
            }

            CleanUp();
        }

        clampSprite(pSpr);
        scrSetMessage("Sprite inserted.");
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_PADPLUS(char key, char ctrl, char shift, char alt)
{
    if (IsHoverSprite())
    {
        spritetype *pSpr = &sprite[searchwall]; XSPRITE* pXSpr;
        int step = (shift) ? 4 : 8;
        char which = 0x03;

        if (key == KEY_PADMINUS)
            step = -step;

        if (ctrl)
            which &= ~0x02;

        if (alt)
            which &= ~0x01;

        switch (pSpr->type)
        {
            case kSoundAmbient:
                GetXSprite(searchwall);
                pXSpr = &xsprite[pSpr->extra];
                if (which & 0x01) pXSpr->data1 = ClipRange(pXSpr->data1 + step, 0, pXSpr->data2);
                if (which & 0x02) pXSpr->data2 = ClipRange(pXSpr->data2 + step, pXSpr->data1, 32767);
                return PROC_OKUB;
            case kMarkerPath:
                if (gModernMap)
                {
                    pSpr->clipdist = ClipRange(pSpr->clipdist + step, 0, 255);
                    return PROC_OKUB;
                }
                break;
            case kModernStealthRegion:
                if (gModernMap)
                {
                    GetXSprite(searchwall);
                    pXSpr = &xsprite[pSpr->extra];
                    pXSpr->data1 = ClipRange(pXSpr->data1 + step, 0, 32767);
                    return PROC_OKUB;
                }
                break;
        }
        
        if (gModernMap && pSpr->extra > 0)
        {
            pXSpr = &xsprite[pSpr->extra];
            if (pXSpr->triggerProximity)
            {
                pSpr->clipdist = ClipRange(pSpr->clipdist + step, 0, 255);
                return PROC_OKUB;
            }
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_END(char key, char ctrl, char shift, char alt)
{
    gScreen2D.prefs.ambRadius = IncRotate(gScreen2D.prefs.ambRadius, 5);
    if (gScreen2D.prefs.ambRadius == 4 && !gScreen2D.prefs.ambRadiusHover)
    {
        gScreen2D.prefs.ambRadiusHover = 1;
        gScreen2D.prefs.ambRadius = 3;
    }
    else
    {
        gScreen2D.prefs.ambRadiusHover = 0;
    }

    return PROC_OKB;
}

int ProcessSprites(HSPRITEFUNC SpriteFunc, int nData, char hglt, char* str)
{
    signed char ask = (hglt & 0x02), r, curStat = -1;
    int nSpr = searchwall, i, n, c = 0;
    
    if (hglt && nSpr >= 0)
    {
        n = hgltSprCount();
        hglt = (sprInHglt(nSpr) && n > 1
            && (!ask || Confirm(batchFormatStr, n - 1, "sprite")));
    }
    
    if (hglt)
    {
        i = highlightcnt;
        while(--i >= 0)
        {
            if ((highlight[i] & 0xC000) == 0)
                continue;
            
            n = highlight[i] & 0x3FFF;
            if ((r = SpriteFunc(&sprite[n], nData)) > 0)
                c++;
            
            if (curStat < 0 && n == nSpr)
                curStat = r;
        }
    }
    else
    {
        if ((curStat = SpriteFunc(&sprite[nSpr], nData)) > 0)
            c = 1;
    }
    
    n = c;
    if (curStat > 0 || c == 0)
    {
        if (str)
            str += sprintf(str, "sprite #%d", nSpr);
        
        if (c > 1)
        {
            if (str)
                str += sprintf(str, "%s", " and ");
            
            c--;
        }
    }
    
    if (str && n > 1)
        str += sprintf(str, "%d other sprites", c);
    
    return c;
}

int ProcessWalls(HWALLFUNC WallFunc, int nData, char both, char hglt, char* str)
{
    signed char ask = (hglt & 0x02), r, curStat = -1;
    int i, n, t, c = 0, numappend = 0;
    int nWall = searchwall;
    
    if (hglt)
    {
        for (OBJECT* o = gListGrd.First(); o->type != OBJ_NONE; o++)
        {
            switch(o->type)
            {
                case OBJ_WALL:
                case OBJ_MASKED:
                    if (wallInHglt(o->index)) break;
                    
                    if (highlightcnt < 0)
                        highlightcnt = 0, numappend++;
                    
                    highlight[highlightcnt++] = o->index;
                    numappend++;
                    break;
            }
        }
        
        n = hgltWallCount();
        hglt = (wallInHglt(nWall) && n > 1
            && (!ask || Confirm(batchFormatStr, n - 1, "wall")));
    }
    
    if (hglt)
    {
        i = highlightcnt;
        while(--i >= 0)
        {
            if ((highlight[i] & 0xC000) != 0)
                continue;
            
            n = highlight[i];
            if ((r = WallFunc(n, nData)) > 0)
            {
                t = wall[n].nextwall;
                if (both && t >= 0 && !wallInHglt(t))
                    WallFunc(t, nData);
                
                c++;
            }
            
            if (curStat < 0 && n == nWall)
                curStat = r;
        }
    }
    else
    {
        if ((curStat = WallFunc(nWall, nData)) > 0)
        {
            t = wall[nWall].nextwall;
            if (both && t >= 0 && WallFunc(t, nData) <= 0)
                both = 0;
            
            c = 1;
        }
    }
    
    t = c;
    if (curStat > 0 || c == 0)
    {
        n = wall[nWall].nextwall;
        
        if (str && !both && !hglt && n >= 0)
            str += sprintf(str, "one side of ");
        
        if (str)
            str += sprintf(str, "wall #%d", nWall);
        
        if (c > 1)
        {
            if (str)
                str += sprintf(str, "%s", " and ");
            
            c--;
        }
    }
    
    if (str && t > 1)
        str += sprintf(str, "%d other walls", c);
    
    highlightcnt -= numappend;
    return c;
}

int ProcessSectors(HSECTORFUNC SectFunc, int nType, int nData, char hglt, char* str)
{
    static char* name[] = { "sector", "floor", "ceiling" };
    int nSect = (ED2D) ? sectorhighlight : searchsector;
    signed char ask = (hglt & 0x02), r, curStat = -1;
    int i, n, c = 0, numappend = 0;
    
    if (nType == OBJ_FLOOR)         nType = 1;
    else if (nType == OBJ_CEILING)  nType = 2;
    else                            nType = 0;
    
    if (hglt)
    {
        for (OBJECT* o = gListGrd.First(); o->type != OBJ_NONE; o++)
        {
            switch(o->type)
            {
                case OBJ_FLOOR:
                case OBJ_CEILING:
                case OBJ_SECTOR:
                    if (sectInHglt(o->index)) break;
                    
                    if (highlightsectorcnt < 0)
                        highlightsectorcnt = 0, numappend++;
                    
                    highlightsector[highlightsectorcnt++] = o->index;
                    numappend++;
                    break;
            }
        }
        
        n = highlightsectorcnt;
        hglt = (sectInHglt(nSect) && n > 1
            && (!ask || Confirm(batchFormatStr, n - 1, name[nType])));
    }
    
    if (hglt)
    {
        i = highlightsectorcnt;
        while(--i >= 0)
        {
            n = highlightsector[i];
            if ((r = SectFunc(n, nData)) > 0)
                c++;
            
            if (curStat < 0 && n == nSect)
                curStat = r;
        }
    }
    else
    {
        if ((curStat = SectFunc(nSect, nData)) > 0)
            c = 1;
    }
    
    n = c;
    if (curStat > 0 || c == 0)
    {
        if (str)
            str += sprintf(str, "%s #%d", name[nType], nSect);
        
        if (c > 1)
        {
            if (str)
                str += sprintf(str, "%s", " and ");
            
            c--;
        }
    }
    
    if (str && n > 1)
        str += sprintf(str, "%d other %ss", c, name[nType]);
            
    highlightsectorcnt -= numappend;
    return c;
}

INPUTPROC gEditInputShared[256] =
{
    { KEY_1,            edKeyProcShared_KEY_1           },
    { KEY_ENTER,        edKeyProcShared_KEY_ENTER       },
    { KEY_ESC,          edKeyProcShared_KEY_ESC         },
    { KEY_TAB,          edKeyProcShared_KEY_TAB         },
    { KEY_DELETE,       edKeyProcShared_KEY_DELETE      },
    { KEY_HOME,         edKeyProcShared_KEY_HOME        },
    { KEY_SCROLLLOCK,   edKeyProcShared_KEY_SCROLLOCK   },
    { KEY_COMMA,        edKeyProcShared_KEY_COMMA       },
    { KEY_PERIOD,       edKeyProcShared_KEY_COMMA       },
    { KEY_PAD5,         edKeyProcShared_KEY_PAD5        },
    { KEY_PAD7,         edKeyProcShared_KEY_PAD7        },
    { KEY_PAD9,         edKeyProcShared_KEY_PAD7        },
    { KEY_PADUP,        edKeyProcShared_KEY_PAD7        },
    { KEY_PADDOWN,      edKeyProcShared_KEY_PAD7        },
    { KEY_PADLEFT,      edKeyProcShared_KEY_PAD7        },
    { KEY_PADRIGHT,     edKeyProcShared_KEY_PAD7        },
    { KEY_PADENTER,     edKeyProcShared_KEY_PADENTER    },

    { KEY_B,            edKeyProcShared_KEY_B           },
    { KEY_D,            edKeyProcShared_KEY_D           },
    { KEY_E,            edKeyProcShared_KEY_E           },
    { KEY_I,            edKeyProcShared_KEY_I           },
    { KEY_J,            edKeyProcShared_KEY_J           },
    { KEY_K,            edKeyProcShared_KEY_K           },
    { KEY_M,            edKeyProcShared_KEY_M           },
    { KEY_Q,            edKeyProcShared_KEY_Q           },
    { KEY_R,            edKeyProcShared_KEY_R           },
    { KEY_U,            edKeyProcShared_KEY_U           },
    { KEY_W,            edKeyProcShared_KEY_W           },
    { KEY_X,            edKeyProcShared_KEY_X           },
    { KEY_Y,            edKeyProcShared_KEY_X           },
    { KEY_Z,            edKeyProcShared_KEY_Z           },
    { KEY_A,            edKeyProcShared_KEY_Z           },

    { KEY_F5,           edKeyProcShared_KEY_F5          },
    { KEY_F6,           edKeyProcShared_KEY_F5          },
    { KEY_F10,          edKeyProcShared_KEY_F10         },
    { KEY_F11,          edKeyProcShared_KEY_F11         },
    { KEY_N,            edKeyProcShared_KEY_N           },
};

INPUTPROC gEditInput3D[256] =
{
    { KEY_A,                edKeyProc3D_KEY_A           },
    { KEY_Z,                edKeyProc3D_KEY_A           },
    { KEY_CAPSLOCK,         edKeyProc3D_KEY_CAPSLOCK    },
    { KEY_INSERT,           edKeyProc3D_KEY_INSERT      },
    { KEY_BACKSPACE,        edKeyProc3D_KEY_BACKSPACE   },
    { KEY_PAGEDN,           edKeyProc3D_KEY_PAGEDN      },
    { KEY_PAGEUP,           edKeyProc3D_KEY_PAGEDN      },
    { KEY_LBRACE,           edKeyProc3D_KEY_RBRACE      },
    { KEY_RBRACE,           edKeyProc3D_KEY_RBRACE      },
    { KEY_BACKSLASH,        edKeyProc3D_KEY_BACKSLASH   },
    { KEY_PLUS,             edKeyProc3D_KEY_PLUS        },
    { KEY_MINUS,            edKeyProc3D_KEY_PLUS        },
    { KEY_C,                edKeyProc3D_KEY_C           },  //
    { KEY_F,                edKeyProc3D_KEY_F           },  //
    { KEY_G,                edKeyProc3D_KEY_G           },  //
    { KEY_H,                edKeyProc3D_KEY_H           },  //
    { KEY_L,                edKeyProc3D_KEY_L           },  //
    { KEY_O,                edKeyProc3D_KEY_O           },
    { KEY_P,                edKeyProc3D_KEY_P           },
    { KEY_S,                edKeyProc3D_KEY_S           },  //
    { KEY_T,                edKeyProc3D_KEY_T           },  //
    { KEY_V,                edKeyProc3D_KEY_V           },
    { KEY_PADMINUS,         edKeyProc3D_KEY_PADMINUS    },  //
    { KEY_PADPLUS,          edKeyProc3D_KEY_PADMINUS    },  //
    { KEY_PAD0,             edKeyProc3D_KEY_PAD0        },
    { KEY_F2,               edKeyProc3D_KEY_F2          },
    { KEY_F3,               edKeyProc3D_KEY_F3          },
    { KEY_F4,               edKeyProc3D_KEY_F3          },
    { KEY_F9,               edKeyProc3D_KEY_F9          },
    { KEY_2,                edKeyProc3D_KEY_2           },
};

INPUTPROC gEditInput2D[256] =
{
    { KEY_BACKSPACE,        edKeyProc2D_KEY_BACKSPACE   },
    { KEY_SPACE,            edKeyProc2D_KEY_SPACE       },
    { KEY_C,                edKeyProc2D_KEY_SPACE       },
    { KEY_INSERT,           edKeyProc2D_KEY_INSERT      },
    { KEY_F8,               edKeyProc2D_KEY_F8          },
    { KEY_F7,               edKeyProc2D_KEY_F7          },
    { KEY_F,                edKeyProc2D_KEY_F           },
    { KEY_H,                edKeyProc2D_KEY_H           },
    { KEY_T,                edKeyProc2D_KEY_T           },
    { KEY_G,                edKeyProc2D_KEY_G           },
    { KEY_L,                edKeyProc2D_KEY_L           },
    { KEY_S,                edKeyProc2D_KEY_S           },
    { KEY_PADPLUS,          edKeyProc2D_KEY_PADPLUS     },
    { KEY_PADMINUS,         edKeyProc2D_KEY_PADPLUS     },
    { KEY_END,              edKeyProc2D_KEY_END         },
    { KEY_PAD1,             edKeyProc2D_KEY_END         },
};

void editInputInit(INPUTPROC* pTable)
{
    INPUTPROC tmp[256], *p;

    memset(tmp,     0,      sizeof(tmp));
    memcpy(tmp,     pTable, sizeof(tmp));
    memset(pTable,  0,      sizeof(tmp));

    for (p = tmp; p->key; p++)
        memcpy(&pTable[p->key], p, sizeof(tmp[0]));
}

void editInputInit()
{
    editInputInit(gEditInputShared);
    editInputInit(gEditInput3D);
    editInputInit(gEditInput2D);
}
