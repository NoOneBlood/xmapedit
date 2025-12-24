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
#include "common_game.h"
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

const char* gTranslucLevNames[3] =
{
    "not",
    "less",
    "mostly",
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

////////////////////////////////////////////////////
/***********************************************/
/** SHARED MAP EDITING INPUT FUNCTIONS        **/
/***********************************************/
//////////////////

static char edKeyProcShared_KEY_1(char key, char ctrl, char shift, char alt)
{
    int i;

    switch (searchstat)
    {
        case OBJ_SPRITE:
            sprite[searchwall].cstat ^= kSprOneSided;
            i = sprite[searchwall].cstat;
            if ((i & kSprRelMask) >= kSprFloor)
            {
                sprite[searchwall].cstat &= ~kSprFlipY;
                if (i & kSprOneSided)
                {
                    if (!ED2D && posz > sprite[searchwall].z)
                        sprite[searchwall].cstat |= kSprFlipY;
                }
            }
            scrSetMessage("Sprite #%d one-sided flag is %s", searchwall, onOff(sprite[searchwall].cstat & kSprOneSided));
            return PROC_OKUB;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            if (ED2D) break;
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            if (wall[searchwall].nextwall >= 0)
            {
                i = wallCstatToggle(searchwall, kWallOneWay, FALSE);
                if ((i & kWallOneWay) && !(i & kWallMasked))
                        wall[searchwall].overpicnum = wall[searchwall].picnum;

                scrSetMessage("Wall #%d one-sided flag is %s", searchwall, onOff(i & kWallOneWay));
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
    char isCeil, ok = 0;

    if (ED3D && gPreviewMode)
    {
        INPUTPROC* pInput = &gEditInput3D[KEY_CAPSLOCK];
        if (pInput->pFunc)
            return pInput->pFunc(key, ctrl, shift, alt);

        return PROC_FAILB;
    }

    if (searchstat == OBJ_NONE)
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
                i = tempidx;
                DIALOG_ITEM* pDlg = (shift || (sector[i].extra > 0 && testXSectorForLighting(sector[i].extra)))
                    ? dlgXSectorFX : dlgXSector;

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
        isCeil = (searchstat == OBJ_CEILING);

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

                    if (isCeil)
                    {
                        pSect->ceilingpicnum    = temppicnum;
                        pSect->ceilingshade     = tempshade;
                        pSect->ceilingpal       = temppal;
                    }
                    else
                    {
                        pSect->floorpicnum      = temppicnum;
                        pSect->floorshade       = tempshade;
                        pSect->floorpal         = temppal;
                    }

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            if (isCeil)
                            {
                                pSect->ceilingxpanning = tempxrepeat;
                                pSect->ceilingypanning = tempyrepeat;
                                pSect->ceilingstat = (BYTE)(tempcstat | kSectParallax);
                            }
                            else
                            {
                                pSect->floorxpanning = tempxrepeat;
                                pSect->floorypanning = tempyrepeat;
                                pSect->floorstat = (BYTE)(tempcstat | kSectParallax);
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
                        pSect->visibility = tempvisibility;

                        if (isCeil)
                        {
                            pSect->ceilingshade = tempshade;
                            pSect->ceilingpal   = temppal;
                        }
                        else
                        {
                            pSect->floorshade   = tempshade;
                            pSect->floorpal = temppal;
                        }
                    }
                }
                else
                {
                    pSect = &sector[searchsector];
                    pSect->visibility = tempvisibility;

                    if (isCeil)
                    {
                        pSect->ceilingshade = tempshade;
                        pSect->ceilingpal   = temppal;
                    }
                    else
                    {
                        pSect->floorshade   = tempshade;
                        pSect->floorpal     = temppal;

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
                            pSect->visibility = tempvisibility;

                            if (isCeil)
                            {
                                pSect->ceilingshade = tempshade;
                                pSect->ceilingpal   = temppal;
                            }
                            else
                            {
                                pSect->floorshade   = tempshade;
                                pSect->floorpal     = temppal;
                            }
                        }
                    }
                }

                return PROC_OKUB;
            }

            if (searchstat == OBJ_SPRITE)
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

                if (isCeil)
                {
                    pSect->ceilingpicnum    = temppicnum;
                    pSect->ceilingshade     = tempshade;
                    pSect->ceilingpal       = temppal;

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            pSect->visibility       = tempvisibility;
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
                else
                {
                    pSect->floorpicnum      = temppicnum;
                    pSect->floorshade       = tempshade;
                    pSect->floorpal         = temppal;

                    switch (somethingintab)
                    {
                        case OBJ_CEILING:
                        case OBJ_FLOOR:
                            pSect->visibility       = tempvisibility;
                            pSect->floorxpanning    = tempxrepeat;
                            pSect->floorypanning    = tempyrepeat;

                            oCstat = sector[searchsector].floorstat;
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

                return PROC_OKUB;
            }

            if (searchstat == OBJ_SPRITE)
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
                        spriteSetSlope(searchwall, tempslope);

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

        if (searchstat == OBJ_SPRITE)
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

    if (searchstat == OBJ_NONE)
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
            tempslope       = 0;

            cpywall[kTabWall] = wall[i];
            if (wall[i].extra > 0)
                cpyxwall[kTabXWall] = xwall[wall[i].extra];

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
            tempang         = sprite[i].ang;
            temptype        = sprite[i].type;
            tempslope       = spriteGetSlope(i);

            cpysprite[kTabSpr] = sprite[i];
            if (sprite[i].extra > 0)
                cpyxsprite[kTabXSpr] = xsprite[sprite[i].extra];

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
                    tempvisibility  = sector[i].visibility;
                    tempslope       = 0;
                    break;
                default:
                    temppicnum      = sector[i].ceilingpicnum;
                    tempshade       = sector[i].ceilingshade;
                    temppal         = sector[i].ceilingpal;
                    tempxrepeat     = sector[i].ceilingxpanning;
                    tempyrepeat     = sector[i].ceilingypanning;
                    tempcstat       = sector[i].ceilingstat;
                    tempextra       = sector[i].extra;
                    tempvisibility  = sector[i].visibility;
                    tempslope       = 0;
                    break;
            }

            if (ED2D)
                searchstat = OBJ_SECTOR; // for names

            cpysector[kTabSect] = sector[searchsector];
            if (sector[searchsector].extra > 0)
                cpyxsector[kTabXSect] = xsector[sector[searchsector].extra];

            break;
    }

    somethingintab = (char)searchstat;
    scrSetMessage("%s[%d] copied in clipboard.", GetHoverName(), i);
    return PROC_OKB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_DELETE(char key, char ctrl, char shift, char alt)
{
    int i = 0, j;

    if (ctrl)
    {
        j = 1, i = -1;
        if (ED3D && (searchstat == OBJ_WALL || searchstat == OBJ_MASKED))
        {
            TranslateWallToSector();
            i = searchsector;
        }

        if (i < 0 && (i = getSector()) < 0) return PROC_FAILB;
        else if (shift || hgltCheck(OBJ_SECTOR, i) < 0) sectDelete(i);
        else j = hgltSectDelete();

        scrSetMessage("%d sector(s) deleted.", j);
        updatesector(posx, posy, &cursectnum);
        updatenumsprites();
        CleanUp();

        return PROC_OKUB;
    }
    else if (searchstat == OBJ_SPRITE)
    {
        spritetype* pSpr = &sprite[searchwall];
        
        if (shift || !sprInHglt(searchwall))
        {
            if (pSpr->statnum == kStatPathMarker && pSpr->extra > 0)
            {
                XSPRITE* pXSpr = &xsprite[pSpr->extra];
                XSPRITE* pXNxt = pathMarkerFind(NULL, pXSpr, 1);
                XSPRITE* pXPrv = pathMarkerFind(NULL, pXSpr, 0);
                
                if (pXNxt || pXPrv)
                    pathMarkerChangeData(pXSpr->data1, (pXNxt) ? pXNxt->data1 : pXPrv->data2);
            }
            
            sprDelete(&sprite[searchwall]);
        }
        else
            i = hgltSprCallFunc(sprDelete);
        
        scrSetMessage("%d sprite(s) deleted.", ClipLow(i, 1));
        updatenumsprites();

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_HOME(char key, char ctrl, char shift, char alt)
{
    int nID, nType, i;
    NAMED_TYPE searchFor[] =
    {
        OBJ_SPRITE,     gSearchStatNames[OBJ_SPRITE],
        OBJ_WALL,       gSearchStatNames[OBJ_WALL],
        OBJ_SECTOR,     gSearchStatNames[OBJ_SECTOR],
    };

    if (ctrl)
    {
        while( 1 )
        {
            if ((nType = showButtons(searchFor, LENGTH(searchFor), "Search for...")) == mrCancel)
                return PROC_FAIL;

            nType -= mrUser;
            sprintf(buffer, "Locate %s #", gSearchStatNames[nType]);
            if ((nID = GetNumberBox(buffer, 0, -1)) >= 0)
                break;
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
                    avePointSector(nID, &posx, &posy);
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

        scrSetMessage("%s #%d %s found", GetHoverName(), nID, isNot(nID >= 0));
        clampCamera();

        return (nID >= 0) ? PROC_OKB : PROC_FAILB;
    }

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
    int i = (shift) ? ((ctrl) ? 128 : 256) : 512;
    int j = 0, s, e, cx, cy, flags = 0;
    char dir = (key == KEY_PERIOD);

    if (!dir)
        i = -i;

    if (ED2D && highlightsectorcnt > 0)
    {
        j = hgltSectRotate(0x04, i);

        if (j > 0)
        {
            scrSetMessage("Rotate %d sectors by %d.", j, i);
            return PROC_OKUB;
        }

        return PROC_OK;
    }

    switch(searchstat)
    {
        case OBJ_SPRITE:
            if (sprInHglt(searchwall))
            {
                hgltSprRotate(i);
                scrSetMessage("Rotate %d sprite(s) by %d", hgltSprCount(), i);
            }
            else
            {
                i = (shift) ? 16 : 256;
                j = sprite[searchwall].ang;
                sprite[searchwall].ang = (short)((dir) ? IncNext(j, i) : DecNext(j, i));

                if (!isMarkerSprite(searchwall))
                    sprite[searchwall].ang = sprite[searchwall].ang & kAngMask;

                scrSetMessage("%s #%d angle: %d", GetHoverName(), searchwall, sprite[searchwall].ang);
            }
            return PROC_OKUB;
        case OBJ_WALL:
        case OBJ_MASKED:
            if (ED3D)
            {
                // temporary rotate
                viewRotateWallTiles(1);

                if (searchwall != searchwall2)  flags |= 0x20;
                if (key == KEY_COMMA)       flags |= 0x10;
                if (!shift)                 flags |= 0x01;
                if (ctrl)                   flags |= 0x04;

                i = AutoAlignWalls(searchwall, flags);
                scrSetMessage("%d walls affected.", i);

                // restore it now
                viewRotateWallTiles(0);

                return (i > 0) ? PROC_OKUB : PROC_OKB;
            }
            break;
    }

    return PROC_OK;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_PAD5(char key, char ctrl, char shift, char alt)
{
    int i = -1;

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
            i = searchwall;
            strcpy(buffer, "repeat");
            sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            i = searchwall;
            strcpy(buffer, "pan/repeat");
            wall[searchwall].xpanning = wall[searchwall].ypanning = 0;
            wall[searchwall].xrepeat  = wall[searchwall].yrepeat  = 8;
            fixrepeats(searchwall);
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            i = searchsector;
            strcpy(buffer, "pan");
            if (searchstat == OBJ_FLOOR) sector[i].floorxpanning = sector[i].floorypanning = 0;
            else sector[i].ceilingxpanning = sector[i].ceilingypanning = 0;
            if (isSkySector(i, searchstat))
            {
                Sky::SetPan(i, searchstat, 0, 0, alt);
                i = -2;
            }
            break;
    }

    if (i >= 0)
        scrSetMessage("%s[%d] %s reset", GetHoverName(), i, buffer);

    return (i >= 0 || i == -2) ? PROC_OKUB : PROC_OKB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_PAD7(char key, char ctrl, char shift, char alt)
{
    walltype* pWall; spritetype* pSpr; sectortype* pSect;
    BOOL bCoarse, chx = 0, chy = 0;
    signed char changedir, step;
    int vlx, vly, i = -1, j;
    char isPan = 0;

    changedir = (key == KEY_PADUP || key == KEY_PADRIGHT || key == KEY_PAD9) ? 1 : -1;
    bCoarse = (!shift) ? TRUE : FALSE;
    step = (bCoarse) ? 8 : 1;

    chx = (key == KEY_PADLEFT || key == KEY_PADRIGHT);
    chy = (key == KEY_PADUP   || key == KEY_PADDOWN);
    if (key == KEY_PAD7 || key == KEY_PAD9)
        chx = chy = TRUE;

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
            pWall = (!ctrl) ? getCorrectWall(searchwall) : &wall[searchwall];
            changedir = -changedir;
            i = searchwall;

            // temporary rotate
            viewRotateWallTiles(1);

            if (ctrl)
            {
                strcpy(buffer, "panning");
                if (chx)
                {
                    // fix wrong panning direction when wall x-flipped and/or bottom swapped.
                    if ((pWall->nextwall >= 0 && (wall[pWall->nextwall].cstat & kWallSwap)
                        && (wall[pWall->nextwall].cstat & kWallFlipX)) || (pWall->cstat & kWallFlipX))
                            changedir = -changedir;

                    pWall->xpanning = changechar(pWall->xpanning, changedir, bCoarse, 0);
                }

                if (chy) pWall->ypanning = changechar(pWall->ypanning, -changedir, bCoarse, 0);

                vlx = pWall->xpanning;
                vly = pWall->ypanning;
                isPan = 1;
            }
            else
            {
                strcpy(buffer, "repeat");
                if (changedir < 0) step = -step;
                if (chx) pWall->xrepeat = ClipRange(pWall->xrepeat + step, 0, 255);
                if (chy) pWall->yrepeat = ClipRange(pWall->yrepeat + step, 0, 255);

                vlx = pWall->xrepeat;
                vly = pWall->yrepeat;
            }

            // restore it now
            viewRotateWallTiles(0);

            break;
        case OBJ_SPRITE:
            pSpr = &sprite[searchwall];
            i = searchwall;
            if (alt)
            {
                strcpy(buffer, "pos");
                if (chx) offsetPos((changedir == 1) ? -step : step, 0, 0, ang, &pSpr->x, &pSpr->y, NULL);
                if (chy) offsetPos(0, (changedir == 1) ? step : -step, 0, ang, &pSpr->x, &pSpr->y, NULL);

                vlx = pSpr->x;
                vly = pSpr->y;
            }
            else if (ctrl)
            {
                strcpy(buffer, "offset");
                if ((pSpr->cstat & kSprRelMask) != kSprSloped)
                {
                    if (chx)
                    {
                        j = -changedir;
                        if (pSpr->cstat & kSprFlipX)
                            j = -j;

                        pSpr->xoffset = changechar(pSpr->xoffset, j, bCoarse, 0);
                    }

                    if (chy)
                    {
                        j = changedir;
                        if ((pSpr->cstat & kSprRelMask) != kSprFace) // fix wrong offset direction when sprite flipped
                        {
                            if (pSpr->cstat & kSprFlipY)
                                j = -j;
                        }

                        pSpr->yoffset = changechar(pSpr->yoffset, j, bCoarse, 0);

                    }
                }

                vlx = pSpr->xoffset;
                vly = pSpr->yoffset;
            }
            else
            {
                strcpy(buffer, "repeat");
                if (changedir < 0) step = -step;
                if (chx) pSpr->xrepeat = ClipRange(pSpr->xrepeat + step, 2, 255);
                if (chy) pSpr->yrepeat = ClipRange(pSpr->yrepeat + step, 2, 255);

                vlx = pSpr->xrepeat;
                vly = pSpr->yrepeat;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            pSect = &sector[searchsector];
            strcpy(buffer, "panning");
            i = searchsector;
            if (searchstat == OBJ_FLOOR)
            {
                if (chx) pSect->floorxpanning = changechar(pSect->floorxpanning, changedir, bCoarse, 0);
                if (chy) pSect->floorypanning = changechar(pSect->floorypanning, changedir, bCoarse, 0);

                vlx = pSect->floorxpanning;
                vly = pSect->floorypanning;
            }
            else
            {
                if (chx) pSect->ceilingxpanning = changechar(pSect->ceilingxpanning, changedir, bCoarse, 0);
                if (chy) pSect->ceilingypanning = changechar(pSect->ceilingypanning, changedir, bCoarse, 0);

                vlx = pSect->ceilingxpanning;
                vly = pSect->ceilingypanning;
            }

            isPan = 1;
            if (isSkySector(searchsector, searchstat))
            {
                i = -2; // show just Sky messages!
                if (chy)
                    Sky::SetPan(searchsector, searchstat, 0, vly, alt);

                if (chx)
                {
                    Sky::FixPan(searchsector, searchstat,  alt); // fix panning first
                    Sky::Rotate((changedir < 0) ? 0 : 1);           // rotate global sky
                }
            }
            else if (alt)
            {
                IDLIST visited(1);
                i = AutoAlignSectors(searchsector, searchstat, &visited);
                return (i > 0) ? PROC_OKUB : PROC_FAILB;
            }

            isPan = 1;
            break;
    }

    if (i >= 0)
        scrSetMessage("%s #%d x%s: %d  y%s: %d", GetHoverName(), i, buffer, vlx, buffer, vly);

    if (i >= 0 || i == -2)
        return (isPan) ? PROC_OKB|PROC_UNDO_ADD : PROC_OKUB;

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
    int i, j, k, f;
    int nWall;


    if (ctrl)
    {
        clipmovemask2d = clipmovemask3d = (clipmovemask3d == 0) ? BLOCK_MOVE : 0;
        scrSetMessage("Block move is %s", onOff(clipmovemask3d));
        return PROC_OKB;
    }

    switch (searchstat)
    {
        case OBJ_SPRITE:
            sprite[searchwall].cstat ^= kSprBlock;
            scrSetMessage("sprite[%d] blocking flag is %s", searchwall, onOff(sprite[searchwall].cstat & kSprBlock));
            return PROC_OKUB;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            if (ED2D) break;
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            if (hgltCheck(OBJ_WALL, searchwall) >= 0)
            {
                k = hgltWallsCheckStat(kWallBlock, 0x02); // red walls has cstat
                f = hgltWallsCheckStat(kWallBlock, 0x06); // red walls has NO cstat
                j = k;

                for (i = 0; i < highlightcnt; i++)
                {
                    if ((highlight[i] & 0xC000) != 0)
                        continue;

                    nWall = highlight[i];
                    if (wall[nWall].nextwall < 0) continue;
                    else if (f < j && f > k)
                    {
                        if (k < j) wallCstatRem(nWall, kWallBlock, !shift);
                        else wallCstatAdd(nWall, kWallBlock, !shift);
                    }
                    else if (k < j) wallCstatAdd(nWall, kWallBlock, !shift);
                    else wallCstatRem(nWall, kWallBlock, !shift);
                }
            }
            else if (wall[searchwall].nextwall >= 0)
            {
                wallCstatToggle(searchwall, kWallBlock, !shift);
            }
            else
            {
                break;
            }

            scrSetMessage("wall[%d] blocking flag is %s", searchwall, onOff(wall[searchwall].cstat & kWallBlock));
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_D(char key, char ctrl, char shift, char alt)
{
    int i;

    if (searchstat == OBJ_SPRITE)
    {
        if (alt)
        {
            sprintf(buffer, "Sprite #%d clipdist", searchwall);
            i = GetNumberBox(buffer, sprite[searchwall].clipdist, sprite[searchwall].clipdist);
            sprite[searchwall].clipdist = (BYTE)ClipRange(i, 0, 255);
            scrSetMessage("%s = %d", buffer, sprite[searchwall].clipdist);
            return PROC_OKUB;
        }
    }

    return PROC_FAIL;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_E(char key, char ctrl, char shift, char alt)
{
    int i;

    if (ED3D)
    {
        if (IsHoverSector())
        {
            sectCstatToggle(searchsector, kSectExpand, searchstat);
            scrSetMessage("%s[%d] texture %s expanded", GetHoverName(), searchsector, isNot(sectCstatGet(searchsector, searchstat) & kSectExpand));
            return PROC_OKUB;
        }
    }

    if (searchstat != OBJ_SPRITE)
        return PROC_FAILB;

    if ((i = GetNumberBox("Enter statnum", sprite[searchwall].statnum, sprite[searchwall].statnum))
        == sprite[searchwall].statnum)
            return PROC_OKB;

    ChangeSpriteStat(searchwall, (short)ClipRange(i, 0, kMaxStatus - 1));
    scrSetMessage("sprite[%d].statnum = %d", searchwall, sprite[searchwall].statnum);
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

    if (searchstat != OBJ_SPRITE)
        return PROC_FAILB;

    sprite[searchwall].cstat ^= kSprInvisible;
    scrSetMessage("sprite[%d] %s invisible", searchwall, isNot(sprite[searchwall].cstat & kSprInvisible));
    return PROC_OKUB;
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
            switch (i = (short)(sprite[searchwall].cstat & kSprMoveMask))
            {
                case kSprMoveNone:
                    i = kSprMoveForward;
                    scrSetMessage("sprite[%d] moves forward (blue).", searchwall);
                    break;
                case kSprMoveForward:
                    i = kSprMoveReverse;
                    scrSetMessage("sprite[%d] moves reverse (green).", searchwall);
                    break;
                case kSprMoveReverse:
                    i = kSprMoveNone;
                    scrSetMessage("sprite[%d] kinetic move disabled.", searchwall);
                    break;
            }
            sprite[searchwall].cstat &= ~kSprMoveMask;
            sprite[searchwall].cstat |= (short)i;
            return PROC_OKUB;
        case OBJ_WALL:
        case OBJ_MASKED:
            switch (i = (short)(wall[searchwall].cstat & kWallMoveMask))
            {
                case kWallMoveNone:
                    i = kWallMoveForward;
                    scrSetMessage("wall[%d] moves forward (blue).", searchwall);
                    break;
                case kWallMoveForward:
                    i = kWallMoveReverse;
                    scrSetMessage("wall[%d] moves reverse (green).", searchwall);
                    break;
                case kWallMoveReverse:
                default:
                    i = kWallMoveNone;
                    scrSetMessage("wall[%d] kinetic move disabled.", searchwall);
                    break;
            }
            wallCstatRem(searchwall, kWallMoveMask, !shift);
            wallCstatAdd(searchwall, i, !shift);
            return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_M(char key, char ctrl, char shift, char alt)
{
    int i;

    if (searchstat == OBJ_SPRITE)
    {
        searchwall = searchwall2;
        searchstat = OBJ_WALL;
    }

    if (searchwall >= 0 && (i = wall[searchwall].nextwall) >= 0)
    {
        if (wallCstatToggle(searchwall, kWallMasked, !shift) & kWallMasked)
        {
            if (!shift)
            {
                // other side flip-x
                if (!(wall[searchwall].cstat & kWallFlipX)) wallCstatAdd(i, kWallFlipX, FALSE);
                else if (wall[i].cstat & kWallFlipX) // other side unflip-x
                    wallCstatRem(i, kWallFlipX, FALSE);

                wall[searchwall].overpicnum = ClipLow(wall[searchwall].overpicnum, 0);
                wall[i].overpicnum = wall[searchwall].overpicnum;
            }

            // useless to have this together
            wallCstatRem(searchwall, kWallOneWay, !shift);
        }

        scrSetMessage("wall[%d] %s masked", searchwall, isNot(wall[searchwall].cstat & kWallMasked));
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_Q(char key, char ctrl, char shift, char alt)
{
    // connect one object with the one from clipboard via RX/TX

    if (somethingintab == OBJ_NONE)
    {
        scrSetMessage("Clipboard is empty!");
        return PROC_FAILB;
    }

    if ((shift && xsysConnect2(somethingintab, tempidx, searchstat, searchindex) < 0)
        || (!shift && xsysConnect(somethingintab, tempidx, searchstat, searchindex) < 0))
                return PROC_FAILB;

    scrSetMessage("Objects connected.");
    CleanUpMisc();

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_R(char key, char ctrl, char shift, char alt)
{
    int i;

    spritetype* pSpr;

    if (ED3D)
    {
        // R (relative alignment, rotation)

        if (IsHoverSector())
        {
            if (!isSkySector(searchsector, searchstat))
            {
                i = sectCstatToggle(searchsector, kSectRelAlign, searchstat);
                scrSetMessage("%s[%d] %s relative", GetHoverName(), searchsector, isNot(i & kSectRelAlign));
                return PROC_OKUB;
            }

            return PROC_FAILB;
        }

        if (searchstat == OBJ_WALL)
        {
            i = wallCstatToggle(searchwall, kWallRotate90, 0);
            scrSetMessage("%s[%d] tile %s rotated 90deg", GetHoverName(), searchwall, isNot(i & kWallRotate90));
            return PROC_OKUB;
        }
    }

    if (searchstat != OBJ_SPRITE)
        return PROC_FAILB;

    pSpr = &sprite[searchwall];
    switch (i = (pSpr->cstat & kSprRelMask))
    {
        case 0x00:
            i = 0x10;
            scrSetMessage("sprite[%d] is wall sprite", searchwall);
            break;
        case 0x10:
            i = 0x20;
            scrSetMessage("sprite[%d] is floor sprite", searchwall);
            break;
        default:
            i = 0x00;
            scrSetMessage("sprite[%d] is face sprite", searchwall);
            break;
    }

    pSpr->cstat &= ~kSprRelMask;
    pSpr->cstat |= (unsigned short)i;

    if (pSpr->cstat & kSprOneSided)
    {
        pSpr->cstat &= ~kSprFlipY;
        if ((pSpr->cstat & kSprRelMask) >= kSprFloor)
        {
            if (posz > pSpr->z)
                pSpr->cstat |= kSprFlipY;
        }
    }

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProcShared_KEY_U(char key, char ctrl, char shift, char alt)
{
    int nVis, i;

    if (IsHoverWall())
        TranslateWallToSector();
    else if (searchstat == OBJ_SPRITE)
        searchsector = sprite[searchwall].sectnum;

    if (searchsector < 0)
        return PROC_FAILB;

    if (ctrl && ED3D)
    {
        nVis = sector[searchsector].visibility;
        if ((nVis = GetNumberBox("Visibility", nVis, -1)) == -1)
            return PROC_FAILB;

        nVis = ClipRange(nVis, 0, 255);
        sector[searchsector].visibility = nVis;

        i = 1;
        if (sectInHglt(searchsector) && Confirm("Set visibility for all sectors in a highlight?"))
        {
            for (i = 0; i < highlightsectorcnt; i++)
                sector[highlightsector[i]].visibility = nVis;
        }

        scrSetMessage("%d sector visibility values are set to %d", i, nVis);
        return PROC_OKUB;
    }

    if (!IsHoverSector())
        return PROC_FAILB;

    GetXSector(searchsector);
    scrSetMessage("sector[%d] is %s underwater", searchsector, isNot(xsector[sector[searchsector].extra].underwater ^= 1));
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
        while ( gWaveNames[nWave] == NULL );
        ProcessHighlightSectors(SetWave, nWave);
        scrSetMessage(gWaveNames[nWave]);
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

    if (searchstat == OBJ_SPRITE)
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

static char edKeyProc3D_KEY_INSERT(char key, char ctrl, char shift, char alt)
{
    spritetype *pSprA, *pSprB;
    int i, j;

    if (searchstat != OBJ_SPRITE)
        return PROC_FAILB;

    i = ClipLow(hgltSprCallFunc(sprClone), 1);
    scrSetMessage("%d sprite(s) duplicated and stamped.", i);
    if (i < 2)
        return PROC_OKUB;

    pSprA = &sprite[searchwall];
    i = highlightcnt;
    while(--i >= 0)
    {
        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = highlight[i] & 0x3FFF;
        pSprB = &sprite[j];
        if (pSprB == pSprA || pSprA->x != pSprB->x
            || pSprA->y != pSprB->y || pSprA->z != pSprB->z)
                continue;

        // give user some time to drag out new sprites
        gObjectLock.type = searchstat;
        gObjectLock.idx  = pSprB->index;
        gObjectLock.time = totalclock + 256;
        gMapedHud.SetMsgImp(256, "Locked on %s #%d", GetHoverName(), pSprB->index);
        break;
    }

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PAGEDN(char key, char ctrl, char shift, char alt)
{
    int nStep = (shift) ? 0x100 : 0x400;
    char d = (key == KEY_PAGEUP);
    int z = 0x80000000, i;

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector() && ctrl && alt)
    {
        if (d)
            nStep = -nStep;

        i = 1;
        if (sectInHglt(searchsector)) i = hgltSectCallFunc(sectChgZ, nStep);
        else sectChgZ(searchsector, nStep);

        if (gMisc.pan)
            AlignSlopes();

        scrSetMessage("Z-offset %d sectors by %d", i, nStep);
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
                ProcessHighlightSectors(SetCeilingZ, z);
                break;
            case OBJ_FLOOR:
                if (d) z = NextSectorNeighborZ(searchsector, sector[searchsector].floorz, 1, 0);
                else z = NextSectorNeighborZ(searchsector, sector[searchsector].floorz, 1, 1);
                ProcessHighlightSectors(SetFloorZ, z);
                break;
            case OBJ_SPRITE:
                hgltSprCallFunc((d) ? PutSpriteOnCeiling : PutSpriteOnFloor, 0);
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
                ProcessHighlightSectors(SetCeilingRelative, z);
                z = sector[searchsector].ceilingz;
                break;
            case OBJ_FLOOR:
                z = dlgZHeightOff("height off ceiling / shift floor by", sector[searchsector].floorz, sector[searchsector].ceilingz, 1, key == KEY_PAGEDN);
                ProcessHighlightSectors(SetFloorRelative, z);
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
    else
    {
        switch(searchstat)
        {
            case OBJ_CEILING:
                ProcessHighlightSectors((d) ? RaiseCeiling : LowerCeiling, nStep);
                z = sector[searchsector].ceilingz;
                break;
            case OBJ_FLOOR:
                ProcessHighlightSectors((d) ? RaiseFloor : LowerFloor, nStep);
                z = sector[searchsector].floorz;
                break;
            case OBJ_SPRITE:
                hgltSprCallFunc((d) ? RaiseSprite : LowerSprite, nStep);
                z = sprite[searchwall].z;
                break;
        }
    }

    if (z != 0x80000000)
    {
        scrSetMessage("%s #%d Z: %d", GetHoverName(), searchindex, z);
        if (gMisc.pan && IsHoverSector())
            AlignSlopes();

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_LBRACE(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect = NULL; spritetype* pSpr = NULL;
    int nStep = (shift) ? 32 : 256;
    int nSlopeA, nSlopeB;
    int i;

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

        char isCeil = (searchstat == OBJ_CEILING);
        strcpy(buffer, GetHoverName());
        strlwr(buffer);

        if (sectInHglt(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                if (isCeil)
                    SetCeilingSlope(highlightsector[i], DecNext(sector[highlightsector[i]].ceilingslope, nStep, -32768));
                else
                    SetFloorSlope(highlightsector[i], DecNext(sector[highlightsector[i]].floorslope, nStep, -32768));
            }

            scrSetMessage("adjusted %d %ss by %d", highlightsectorcnt, buffer, nStep);
        }
        else
        {
            if (isCeil)
                SetCeilingSlope(searchsector, DecNext(pSect->ceilingslope, nStep, -32768));
            else
                SetFloorSlope(searchsector, DecNext(pSect->floorslope, nStep, -32768));

            scrSetMessage("sector[%d].%sslope: %d", searchsector, buffer, (isCeil) ? pSect->ceilingslope : pSect->floorslope);
        }

        return PROC_OKUB;
    }

    if (searchstat == OBJ_SPRITE)
    {
        pSpr = &sprite[searchwall];
        switch(pSpr->cstat & kSprRelMask)
        {
            case kSprFloor:
            case kSprSloped:
                nSlopeB = spriteGetSlope(searchwall);
                nSlopeA = 0;

                if (ctrl)       nSlopeA = sector[pSpr->sectnum].floorslope;
                else if (alt)   nSlopeA = sector[pSpr->sectnum].ceilingslope;
                else            nSlopeA = (short)DecNext(nSlopeB, nStep, -32768);

                spriteSetSlope(searchwall, nSlopeA);
                scrSetMessage("sprite[%d].slope: %d", searchwall, nSlopeA);
                return PROC_OKUB;
            default:
                scrSetMessage("sprite[%d] must be floor aligned!", searchwall);
                break;

        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_RBRACE(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect = NULL; spritetype* pSpr = NULL;
    int nStep = (shift) ? 32 : 256;
    int nSlopeA, nSlopeB, nNext;
    int x, y, i;

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

        char isCeil = (searchstat == OBJ_CEILING);
        strcpy(buffer, GetHoverName());
        strlwr(buffer);

        if (alt)
        {
            if ((nNext = wall[searchwall].nextsector) < 0)
                return PROC_FAILB;

            x = wall[searchwall].x;
            y = wall[searchwall].y;

            if (isCeil) alignceilslope(searchsector, x, y, getceilzofslope(nNext, x, y));
            else alignflorslope(searchsector, x, y, getflorzofslope(nNext, x, y));
            return PROC_OKUB;
        }

        if (sectInHglt(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                if (isCeil)
                    SetCeilingSlope(highlightsector[i], IncNext(sector[highlightsector[i]].ceilingslope, nStep, 32767));
                else
                    SetFloorSlope(highlightsector[i], IncNext(sector[highlightsector[i]].floorslope, nStep, 32767));
            }

            scrSetMessage("adjusted %d %ss by %d", highlightsectorcnt, buffer, nStep);
        }
        else
        {
            if (isCeil)
                SetCeilingSlope(searchsector, IncNext(pSect->ceilingslope, nStep, 32767));
            else
                SetFloorSlope(searchsector, IncNext(pSect->floorslope, nStep, 32767));

            scrSetMessage("sector[%d].%sslope: %d", searchsector, buffer, (isCeil) ? pSect->ceilingslope : pSect->floorslope);
        }

        return PROC_OKUB;

    }

    if (searchstat == OBJ_SPRITE)
    {
        pSpr = &sprite[searchwall];
        switch(pSpr->cstat & kSprRelMask)
        {
            case kSprFloor:
            case kSprSloped:
                nSlopeB = spriteGetSlope(searchwall);
                nSlopeA = 0;

                if (ctrl)       nSlopeA = sector[pSpr->sectnum].floorslope;
                else if (alt)   nSlopeA = sector[pSpr->sectnum].ceilingslope;
                else            nSlopeA = (short)IncNext(nSlopeB, nStep, 32767);

                spriteSetSlope(searchwall, nSlopeA);
                scrSetMessage("sprite[%d].slope: %d", searchwall, nSlopeA);
                return PROC_OKUB;
            default:
                scrSetMessage("sprite[%d] must be floor aligned!", searchwall);
                break;

        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_BACKSLASH(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect = NULL; spritetype* pSpr = NULL;
    int i;

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

        char isCeil = (searchstat == OBJ_CEILING);
        strcpy(buffer, GetHoverName());
        strlwr(buffer);

        if (sectInHglt(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                if (isCeil) SetCeilingSlope(highlightsector[i], 0);
                else        SetFloorSlope(highlightsector[i], 0);
            }

            scrSetMessage("slope reset for %d %ss", highlightsectorcnt, buffer);
        }
        else
        {
            if (isCeil) SetCeilingSlope(searchsector, 0);
            else        SetFloorSlope(searchsector, 0);

            scrSetMessage("sector[%d] %s slope reset", searchsector, buffer);
        }

        return PROC_OKUB;
    }

    if (searchstat == OBJ_SPRITE)
    {
        pSpr = &sprite[searchwall];
        switch(pSpr->cstat & kSprRelMask)
        {
            case kSprFloor:
            case kSprSloped:
                spriteSetSlope(searchwall, 0);
                scrSetMessage("sprite[%d] slope reset", searchwall);
                return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_PLUS(char key, char ctrl, char shift, char alt)
{
    int nStep = (key == KEY_MINUS) ? 1 : -1, nXSect, i;
    strcpy(buffer, (nStep < 0) ? "more" : "less");

    if (keystatus[KEY_D])
    {
        nStep = mulscale10((key == KEY_MINUS) ? 16 : -16, numpalookups<<5);
        visibility = ClipRange(visibility + nStep, 0, kMaxVisibility);
        scrSetMessage("Global visibility %d (%s)", visibility, buffer);
        return PROC_OKUB;
    }

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector())
    {
        if (ctrl && alt)
        {
            // iterate sector visibility (higher numbers are less)

            if (sectInHglt(searchsector))
            {
                i = hgltSectCallFunc((HSECTORFUNC2*)sectChgVisibility, nStep);
                scrSetMessage("%d sectors are %s visible", i, buffer);
            }
            else
            {
                sectChgVisibility(searchsector, nStep);
                scrSetMessage("sector[%d] visibility: %d (%s)", searchsector, sector[searchsector].visibility, buffer);
            }

            return PROC_OKUB;
        }

        if (ctrl)
        {
            // iterate lighting effect amplitude
            nXSect = GetXSector(searchsector);
            xsector[nXSect].amplitude  += nStep;
            scrSetMessage("Amplitude: %d", xsector[nXSect].amplitude);
            return PROC_OKUB;
        }

        if (shift)
        {
            // iterate lighting effect phase
            nXSect = GetXSector(searchsector);
            xsector[nXSect].shadePhase += nStep;
            scrSetMessage("Phase: %d", xsector[nXSect].shadePhase);
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_C(char key, char ctrl, char shift, char alt)
{
    // change all tiles matching target to tab picnum
    sectortype* pSect; walltype* pWall; spritetype* pSpr;
    int i, j;

    if (!alt || somethingintab != searchstat)
        return PROC_FAILB;

    if (IsHoverWall())
    {
        pWall = &wall[searchwall];
        j = (searchstat == OBJ_WALL)
                ? pWall->picnum : pWall->overpicnum;

        if (wallInHglt(searchwall))
        {
            for (i = 0; i < highlightcnt; i++)
            {
                if ((highlight[i] & 0xC000) != 0)
                    continue;

                pWall = &wall[highlight[i]];
                if (searchstat == OBJ_WALL)
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
                if (searchstat == OBJ_WALL)
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
        j = (searchstat == OBJ_FLOOR)
                ? pSect->floorpicnum : pSect->ceilingpicnum;

        if (sectInHglt(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                pSect = &sector[highlightsector[i]];
                if (searchstat == OBJ_CEILING)
                {
                    if (pSect->ceilingpicnum != j)                  continue;
                    else if (pSect->ceilingpicnum != temppicnum)    pSect->ceilingpicnum = temppicnum;
                    else if (pSect->ceilingpal != temppal)          pSect->ceilingpal = temppal;
                }
                else
                {
                    if (pSect->floorpicnum != j)                    continue;
                    else if (pSect->floorpicnum != temppicnum)      pSect->floorpicnum = temppicnum;
                    else if (pSect->floorpal != temppal)            pSect->floorpal = temppal;
                }
            }
        }
        else
        {
            for (i = 0; i < numsectors; i++)
            {
                pSect = &sector[i];
                if (searchstat == OBJ_CEILING)
                {
                    if (pSect->ceilingpicnum != j)                  continue;
                    else if (pSect->ceilingpicnum != temppicnum)    pSect->ceilingpicnum = temppicnum;
                    else if (pSect->ceilingpal != temppal)          pSect->ceilingpal = temppal;
                }
                else
                {
                    if (pSect->floorpicnum != j)                    continue;
                    else if (pSect->floorpicnum != temppicnum)      pSect->floorpicnum = temppicnum;
                    else if (pSect->floorpal != temppal)            pSect->floorpal = temppal;
                }
            }
        }

        return PROC_OKUB;
    }

    if (searchstat == OBJ_SPRITE)
    {
        j = sprite[searchwall].picnum;
        for (i = 0; i < kMaxSprites; i++)
        {
            pSpr = &sprite[i];
            if (pSpr->statnum >= kMaxStatus
                || pSpr->picnum != j)
                    continue;

            pSpr->picnum = temppicnum;
            pSpr->type = temptype;
        }

        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect; walltype* pWall; spritetype* pSpr;
    int zTopOld, zTopNew;
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

        switch((i = (pWall->cstat & kWallFlipMask)))
        {
            case 0:                     i = kWallFlipX;                 break;
            case kWallFlipX:            i = kWallFlipX|kWallFlipY;      break;
            case kWallFlipX|kWallFlipY: i = kWallFlipY;                 break;
            case kWallFlipY:            i = 0;                          break;
        }

        pWall->cstat &= ~kWallFlipMask;
        pWall->cstat |= (short)i;

        sprintf(buffer, "wall[%d]", searchwall);
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
        pSect = &sector[searchsector];
        i = (searchstat == OBJ_CEILING)
            ? pSect->ceilingstat : pSect->floorstat;

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

        sectCstatRem(searchsector, kSectFlipMask, searchstat);
        sectCstatAdd(searchsector, i, searchstat);
        return PROC_OKUB;
    }

    if (searchstat == OBJ_SPRITE)
    {
        pSpr = &sprite[searchwall];
        GetSpriteExtents(pSpr, &zTopOld, &i);
        i = pSpr->cstat;

        // two-sided floor sprite?
        if ((i & kSprRelMask) == kSprFloor && !(i & kSprOneSided))
        {
            // what the hell is this supposed to be doing?
            pSpr->cstat &= ~kSprFlipY;
            pSpr->cstat ^= kSprFlipX;
        }
        else
        {
            i = i & 0xC;
            switch(i)
            {
                case 0x0: i = 0x4; break;
                case 0x4: i = 0xC; break;
                case 0xC: i = 0x8; break;
                case 0x8: i = 0x0; break;
            }

            pSpr->cstat &= ~0xC;
            pSpr->cstat |= (short)i;
        }

        sprintf(buffer, "sprite[%d]", searchwall);
        if (pSpr->cstat & kSprFlipX)
            strcat(buffer," x-flipped");

        if (pSpr->cstat & kSprFlipY)
        {
            if (pSpr->cstat & kSprFlipX)
                strcat(buffer," and");

            strcat(buffer," y-flipped");
        }

        scrSetMessage(buffer);
        GetSpriteExtents(pSpr, &zTopNew, &i);
        pSpr->z += (zTopOld-zTopNew); // compensate Z (useful for wall sprites)
        return PROC_OKUB;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_G(char key, char ctrl, char shift, char alt)
{
    if (shift || (!alt && gHighSpr >= 0))
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

    if (gListGrd.Exists(searchstat, searchindex))
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
    // H (hitscan sensitivity)

    switch (searchstat)
    {
        case OBJ_FLOOR:
        case OBJ_CEILING:
            // no break
        case OBJ_WALL:
        case OBJ_MASKED:
            wallCstatToggle(searchwall, kWallHitscan, !shift);
            scrSetMessage("wall[%d] %s hitscan sensitive", searchwall, isNot(wall[searchwall].cstat & kWallHitscan));
            return PROC_OKUB;
        case OBJ_SPRITE:
            sprite[searchwall].cstat ^= kSprHitscan;
            scrSetMessage("sprite[%d] %s hitscan sensitive", searchwall, isNot(sprite[searchwall].cstat & kSprHitscan));
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

    if (searchstat == OBJ_SPRITE)
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
        pWall->cstat ^= kWallOrgBottom;
        if (pWall->nextwall == -1) scrSetMessage("Texture pegged at %s", (pWall->cstat & kWallOrgBottom) ? "bottom" : "top");
        else scrSetMessage("Texture pegged at %s", (pWall->cstat & kWallOrgOutside) ? "outside" : "inside");
        return PROC_OKUB;
    }

    if (searchstat == OBJ_SPRITE)
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
            pSpr->cstat ^= kSprOrigin;
            clampSprite(pSpr);
            
            scrSetMessage("%s[%d] origin align is %s", GetHoverName(), searchwall, onOff((pSpr->cstat & kSprOrigin)));
            return PROC_OKUB;
        }
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_P(char key, char ctrl, char shift, char alt)
{
    char title[64], nPlu;
    int nPic, nShade;
    int i;

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
            else if (searchstat == OBJ_CEILING)         sectCstatRem(searchsector, kSectShadeFloor, searchstat);
            scrSetMessage("%s[%d] %s parallaxed", GetHoverName(), searchsector, isNot(isSkySector(searchsector, searchstat)));
            return PROC_OKUB;
        }
    }

    nPlu    = getPluOf(searchstat,      searchindex);
    nPic    = getPicOf(searchstat,      searchindex);
    nShade  = getShadeOf(searchstat,    searchindex);
    sprintf(title, "%s #%d palookup", GetHoverName(), searchindex);

    if (!shift)
    {
        i = nPlu;
        if (searchstat == OBJ_SPRITE)
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
        sprintf(buffer, "(%d efficiency)", i);

    scrSetMessage("%s: #%d %s", strlwr(title), nPlu, buffer);
    if (searchstat == OBJ_SPRITE && sprInHglt(searchindex) && !shift)
        hgltSprCallFunc(sprPalSet, nPlu);
    else
        setPluOf(nPlu, searchstat, searchindex);

    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_S(char key, char ctrl, char shift, char alt)
{
    short hitsect = -1, hitwall = -1, hitsprite = -1;
    int i, x, y, z;
    int nPic;

    spritetype* pSpr;

    if (searchstat == OBJ_SPRITE
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
                if ((pSpr->cstat & kSprRelMask) == kSprSloped)
                    spriteSetSlope(i, tempslope);
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
    int i = 0, j;

    buffer[0] = '\0';
    switch (searchstat)
    {
        case OBJ_MASKED:
            sprintf(buffer, "%d", searchwall);
            if (wall[searchwall].cstat & kWallTransluc)  i = 2;
            if (wall[searchwall].cstat & kWallTranslucR) i = 1;
            switch (i = IncRotate(i, 3))
            {
                case 0:
                    wallCstatRem(searchwall, kWallTransluc2, !shift);
                    break;
                case 1:
                    wallCstatAdd(searchwall, kWallTransluc2, !shift);
                    break;
                case 2:
                    wallCstatRem(searchwall, kWallTransluc2, !shift);
                    wallCstatAdd(searchwall, kWallTransluc,  !shift);
                    break;
            }
            break;
        case OBJ_SPRITE:
            sprintf(buffer, "%d", searchwall);
            if (sprite[searchwall].cstat & kSprTransluc1) i = 2;
            if (sprite[searchwall].cstat & kSprTranslucR) i = 1;
            switch (i = IncRotate(i, 3))
            {
                case 0:
                    sprite[searchwall].cstat &= ~kSprTransluc2;
                    break;
                case 1:
                    sprite[searchwall].cstat |= kSprTransluc2;
                    break;
                case 2:
                    sprite[searchwall].cstat &= ~kSprTransluc2;
                    sprite[searchwall].cstat |= kSprTransluc1;
                    break;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            sprintf(buffer, "%d", searchsector);
            j = sectCstatGet(searchsector, searchstat);
            switch (j & kSectTranslucR)
            {
                case 0: // first set it just masked
                    sectCstatAdd(searchsector, kSectMasked, searchstat);
                    scrSetMessage("%s[%s] is masked", GetHoverName(), buffer);
                    buffer[0] = '\0';
                    break;
                case kSectMasked:
                    sectCstatAdd(searchsector, kSectTransluc, searchstat); // less
                    i = 1;
                    break;
                case kSectTranslucR:
                    sectCstatAdd(searchsector, kSectTranslucR, searchstat); // mostly
                    sectCstatRem(searchsector, kSectMasked, searchstat);
                    i = 2;
                    break;
                case kSectTransluc:
                    sectCstatRem(searchsector, kSectTranslucR, searchstat); // none
                    scrSetMessage("%s[%s] is not masked nor translucent", GetHoverName(), buffer);
                    buffer[0] = '\0';
                    break;
            }
            break;
        default:
            return PROC_FAILB;
    }

    if (buffer[0] != '\0')
        scrSetMessage("%s[%s] %s tanslucent", GetHoverName(), buffer, gTranslucLevNames[i]);

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

            if (alt)        nPic = pSpr->picnum = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = pSpr->picnum = nextTile(oPic, flags);
            else            nPic = pSpr->picnum = tilePick(oPic, oPic, searchstat, buffer);
            break;

        case OBJ_WALL:

            pWall = &wall[searchwall];
            oPic = pWall->picnum;

            if (alt)        nPic = pWall->picnum = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = pWall->picnum = nextTile(oPic, flags|0x06);
            else            nPic = pWall->picnum = tilePick(oPic, oPic, searchstat, buffer);
            break;

        case OBJ_MASKED:

            pWall = &wall[searchwall];
            oPic = pWall->overpicnum;

            if (alt)        nPic = pWall->overpicnum = favoriteTileSelect(oPic, oPic, searchstat);
            else if (shift) nPic = pWall->overpicnum = nextTile(oPic, flags|0x04);
            else            nPic = pWall->overpicnum = tilePick(oPic, oPic, searchstat, buffer);

            if (pWall->nextwall >= 0) wall[pWall->nextwall].overpicnum = nPic;
            break;

        case OBJ_CEILING:
        case OBJ_FLOOR:

            pSect = &sector[searchsector];
            if (searchstat == OBJ_FLOOR)
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

            if (isSkySector(searchsector, searchstat))
            {
                Sky::SetPic(searchsector, searchstat, nPic, !gMisc.diffSky);
            }
            else if (searchstat == OBJ_FLOOR)
            {
                pSect->floorpicnum = nPic;
            }
            else
            {
                pSect->ceilingpicnum = nPic;
                pSect->floorstat &= ~kSectShadeFloor;   // clear forced floor shading bit
            }

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

    if (gListGrd.Exists(searchstat, searchindex))
    {
        OBJECT* pFirst = gListGrd.Ptr();
        OBJECT* pDb;

        // check if at least one of highlighted objects gets reached min/max shade
        if (nStep > 0)
        {
            for (pDb = pFirst; pDb->type != OBJ_NONE; pDb++)
            {
                if (getShadeOf(pDb->type, pDb->index) < NUMPALOOKUPS(1)) continue;
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
            if (!shift && sprInHglt(searchwall))
            {
                scrSetMessage("%d sprites darker by %d", hgltSprCallFunc(sprShadeIterate, nStep), nStep);
            }
            else
            {
                sprShadeIterate(&sprite[searchwall], nStep);
                scrSetMessage("Shade: %d", sprite[searchwall].shade);
            }
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
                (pal) ? wall[searchwall].pal = 0 : wall[searchwall].shade = 0;
                break;
            case OBJ_FLOOR:
            case OBJ_CEILING:
                if (pal)
                {
                    if (searchstat == OBJ_CEILING)
                        sector[searchsector].ceilingpal = 0;
                    else
                        sector[searchsector].floorpal = 0;

                    if (isSkySector(searchsector, searchstat))
                        Sky::SetPal(searchsector, searchstat, 0, alt);
                }
                else
                {
                    if (searchstat == OBJ_CEILING)
                        sector[searchsector].ceilingshade = 0;
                    else
                        sector[searchsector].floorshade = 0;

                    if (isSkySector(searchsector, searchstat))
                        Sky::SetShade(searchsector, searchstat, 0, alt);
                }
                break;
            case OBJ_SPRITE:
                if (!shift) hgltSprCallFunc((pal) ? sprPalSet : sprShadeSet, 0);
                else (pal) ? sprite[searchwall].pal = 0 : sprite[searchwall].shade = 0;
                break;
        }
    }

    scrSetMessage("%s reset.", (pal) ? "Palette" : "Shade");
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
                xwall[nID].state ^= 1;
                xwall[nID].busy = xwall[nID].state << 16;
                scrSetMessage("%s[%d].state is %s", GetHoverName(), searchwall, onOff(xwall[nID].state));
                return PROC_OKUB;
            }
            break;
        case OBJ_CEILING:
        case OBJ_FLOOR:
            if ((nID = sector[searchsector].extra) > 0)
            {
                xsector[nID].state ^= 1;
                xsector[nID].busy = xsector[nID].state << 16;
                scrSetMessage("%s[%d].state is %s", GetHoverName(), searchsector, onOff(xsector[nID].state));
                return PROC_OKUB;
            }
            break;
        case OBJ_SPRITE:
            if ((nID = sprite[searchwall].extra) > 0)
            {
                xsprite[nID].state ^= 1;
                xsprite[nID].busy = xsprite[nID].state << 16;
                scrSetMessage("%s[%d].state is %s", GetHoverName(), searchwall, onOff(xsprite[nID].state));
                return PROC_OKUB;
            }
            break;
    }

    return PROC_FAILB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc3D_KEY_F3(char key, char ctrl, char shift, char alt)
{
    sectortype* pSect; XSECTOR* pXSect;

    if (IsHoverWall())
        TranslateWallToSector();

    if (IsHoverSector())
    {
        pSect = &sector[searchsector];
        switch (pSect->type)
        {
            default:
                scrSetMessage("Sector type #%d does not support z-motion!", pSect->type);
                return PROC_FAILB;
            case kSectorZMotionSprite:
                if (!alt)
                {
                    scrSetMessage("Preview unavailable for %s!", gSectorNames[pSect->type]);
                    return PROC_FAILB; // you can't preview because of sprites...
                }
                // no break
            case kSectorZMotion:
            case kSectorRotate:
            case kSectorRotateMarked:
            case kSectorSlide:
            case kSectorSlideMarked:
                pXSect = &xsector[GetXSector(searchsector)];
                if (key == KEY_F3)
                {
                    if (alt)
                    {
                        pXSect->offFloorZ = pSect->floorz;
                        pXSect->offCeilZ  = pSect->ceilingz;
                    }

                    pSect->floorz   = pXSect->offFloorZ;
                    pSect->ceilingz = pXSect->offCeilZ;
                }
                else
                {
                    if (alt)
                    {
                        pXSect->onFloorZ = pSect->floorz;
                        pXSect->onCeilZ  = pSect->ceilingz;
                    }

                    pSect->floorz   = pXSect->onFloorZ;
                    pSect->ceilingz = pXSect->onCeilZ;
                }

                sprintf(buffer, "%s %s", (alt) ? "Capture" : "Show", onOff(key == KEY_F4));
                scrSetMessage("%s floorZ=%d, ceilingZ=%d", strupr(buffer), pSect->floorz, pSect->ceilingz);
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

    walltype *pWallA, *pWallB;

    if (searchstat == OBJ_SPRITE
        || searchwall < 0 || wall[searchwall].nextwall < 0)
                return PROC_FAILB;

    pWallA  = &wall[searchwall];
    pWallB = getCorrectWall(searchwall);

    if (pWallB->nextwall == searchwall)
        pWallA = pWallB;

    pWallA->cstat ^= kWallSwap;
    scrSetMessage("wall[%d] bottom swap flag is %s", searchwall, onOff(pWallA->cstat & kWallSwap));
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

        if (shift && pointhighlight >= 0
            && (pointhighlight & 0xc000) == 0)
                getWallCoords(pointhighlight, &x, &y);

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
    int nGrid = (grid <= 0) ? 10 : grid;
    int nSect, i, j, s, e;
    int x, y;

    if (highlightsectorcnt > 0)
    {
        i = highlightsectorcnt;
        while(--i >= 0)
        {
            nSect = highlightsector[i];
            sectClone(nSect, numsectors, numwalls);
            sectChgXY(numsectors, 2048>>nGrid, 2048>>nGrid, 0x03);

            getSectorWalls(nSect, &s, &e);
            j = sector[numsectors].wallptr;

            while(s <= e)
            {
                gNextWall[j] = wall[s].nextwall;
                wallDetach(j);

                numwalls++;
                s++, j++;
            }

            highlightsector[i] = numsectors++;
        }

        hgltSectAttach(); // call this to attach the sectors inside highlight
        scrSetMessage("%d sector(s) duplicated and stamped.", highlightsectorcnt);
        return PROC_OKUB;
    }

    if (highlightcnt > 0 || searchstat == OBJ_SPRITE)
    {
        if ((highlightcnt > 0 && searchstat == OBJ_SPRITE && sprInHglt(searchwall)) || searchstat == OBJ_SPRITE)
        {
            scrSetMessage("%d sprite(s) duplicated and stamped.", ClipLow(hgltSprCallFunc(sprClone), 1));
            if (pointdrag >= 0 && (pointdrag & 0xC000) != 0)
            {
                i = pointdrag & 0x3FFF;
                if (!sprInHglt(i))
                {
                    x = sprite[i].x;
                    y = sprite[i].y;

                    i = -1;
                    while(nextSpriteAt(x, y, &i) >= 0)
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
        else if (searchstat == OBJ_SPRITE) scrSetMessage("Must aim in objects in a highlight.");
        else if (searchstat != OBJ_NONE) scrSetMessage("Must have no objects in a highlight.");
        return PROC_FAILB;
    }

    if (linehighlight >= 0)
    {
        x = mousxplc, y = mousyplc;
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
    switch (searchstat)
    {
        case OBJ_WALL:
        case OBJ_SPRITE:
            CXTracker::Track(searchstat, searchwall, i);
            break;
        case OBJ_FLOOR:
            CXTracker::Track(OBJ_SECTOR, sectorhighlight, i);
            break;
    }

    return PROC_OKB;
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
    if (linehighlight < 0 || hgltWallCount())
        return PROC_FAILB;

    setFirstWall(sectorofwall(linehighlight), linehighlight);
    scrSetMessage("The wall %d now sector's %d first wall.", linehighlight, sectorofwall(linehighlight));
    return PROC_OKUB;
}

// ----------------------------------------------------------------------------- //

static char edKeyProc2D_KEY_H(char key, char ctrl, char shift, char alt)
{
    int i;

    if (searchstat == OBJ_NONE)
        return PROC_FAILB;

    if (searchstat != OBJ_FLOOR) i = searchwall;
    else i = searchsector, searchstat = OBJ_SECTOR;
    sprintf(buffer, "%s #%d %s: ", GetHoverName(), i, (key == KEY_T) ? "lo-tag" : "hi-tag");

    switch (searchstat)
    {
        case OBJ_WALL:
            if (key == KEY_H)
            {
                if (ctrl) wall[i].hitag = ClipRange(GetNumberBox(buffer, wall[i].hitag, wall[i].hitag), 0, 32767);
                else if (wall[i].nextwall >= 0)
                {
                    wallCstatToggle(i, kWallHitscan, !shift);
                    scrSetMessage("Wall[%d] %s hitscan sensitive", i, isNot(wall[i].cstat & kWallHitscan));
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
                    sprite[i].cstat ^= kSprHitscan;
                    scrSetMessage("Sprite[%d] %s hitscan sensitive", i, isNot(sprite[i].cstat & kSprHitscan));
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
    int nGrid, i, j;

    if (ctrl && alt)
    {
        if ((i = hgltWallCount()) > 0)
        {
            nGrid = grid;
            sprintf(buffer, "Snap %d wall points to grid... (%d - %d)", i, 1, kMaxGrids);
            if ((nGrid = GetNumberBox(buffer, nGrid, 0)) > 0)
            {
                i = highlightcnt;
                while(--i >= 0)
                {
                    j = highlight[i];
                    if ((j & 0xC000) != 0)
                        continue;

                    doGridCorrection(&wall[j].x, &wall[j].y, nGrid);
                }

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
        
            pSpr->picnum  = cpysprite[kTabSpr].picnum;
            pSpr->shade   = cpysprite[kTabSpr].shade;
            pSpr->pal     = cpysprite[kTabSpr].pal;
            pSpr->xrepeat = cpysprite[kTabSpr].xrepeat;
            pSpr->yrepeat = cpysprite[kTabSpr].yrepeat;
            pSpr->xoffset = cpysprite[kTabSpr].xoffset;
            pSpr->yoffset = cpysprite[kTabSpr].yoffset;

            if ((cpysprite[kTabSpr].cstat & kSprRelMask) == kSprSloped)
                cpysprite[kTabSpr].cstat &= ~kSprSloped;

            pSpr->cstat = cpysprite[kTabSpr].cstat;

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
                        sprite[i].ang = cpysprite[kTabSpr].ang;
                    }
                }

                // make a loop?
                if (searchstat == OBJ_SPRITE && sprite[searchwall].type == kMarkerPath)
                {
                    DeleteSprite(i);
                    xsprite[k].data2 = xsprite[sprite[searchwall].extra].data1;
                    if (!Confirm("Finish path drawing now?"))
                    {
                        cpysprite[kTabSpr]   = sprite[searchwall];
                        cpyxsprite[kTabXSpr] = xsprite[searchwall];
                        tempidx = searchwall;
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
                                cpysprite[kTabSpr]   = sprite[searchwall];
                                cpyxsprite[kTabXSpr] = xsprite[searchwall];
                                tempidx = searchwall;
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
                    cpysprite[kTabSpr]   = sprite[i];
                    cpyxsprite[kTabXSpr] = xsprite[j];
                    tempidx = (short)i;
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
    if (searchstat == OBJ_SPRITE)
    {
        spritetype *pSpr = &sprite[searchwall]; XSPRITE* pXSpr;
        int step = (shift) ? 10 : 20;
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
                ShowSpriteData(searchwall, TRUE);
                return PROC_OKUB;
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
};

INPUTPROC gEditInput3D[256] =
{
    { KEY_A,                edKeyProc3D_KEY_A           },
    { KEY_Z,                edKeyProc3D_KEY_A           },
    { KEY_CAPSLOCK,         edKeyProc3D_KEY_CAPSLOCK    },
    { KEY_INSERT,           edKeyProc3D_KEY_INSERT      },
    { KEY_PAGEDN,           edKeyProc3D_KEY_PAGEDN      },
    { KEY_PAGEUP,           edKeyProc3D_KEY_PAGEDN      },
    { KEY_LBRACE,           edKeyProc3D_KEY_LBRACE      },
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
