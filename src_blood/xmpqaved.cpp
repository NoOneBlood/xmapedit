/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Implementation of QAV animation files editor (QAVEDIT).
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
#include "tile.h"
#include "xmptools.h"
#include "xmpqaved.h"
#include "xmpmaped.h"
#include "preview.h"

QAVEDIT gQaved;

enum {
kQavOriginPlayer        = 0,
kQavOriginScreen        = 1,
kQavOriginCustom        = 2,
};

POINT2D gQavOriginTypes[] = {

    {160, 199},
    {160, 100},

};

char* gQavOriginNames[] =  {

    "Player origin",
    "Screen origin",
    "Custom origin",

};

NAMED_TYPE gImportOptions[] = {

    {0, "&Put new frames in the end" },
    {1, "&Enter frame number..." },

};

void QAVEDIT::Start(char* filename)
{
    int i, screenMode = qsetmode;

    AnimNew();
    if (filename)
        strcpy(gPaths.qavs, filename);

    if (!filename || !AnimLoad(gPaths.qavs))
    {
        while ( 1 )
        {
            switch (showButtons(toolMenu, LENGTH(toolMenu), gToolNames[kToolQavEdit].name) - mrUser) {
                case 1:
                    if (!toolLoadAs(gPaths.qavs, kQavExt)) continue;
                    else if (AnimLoad(gPaths.qavs)) break;
                    else Alert("Failed to load file \"%s\"!", gPaths.qavs);
                    continue;
                case 0:
                    if ((i = AnimOriginSelect()) >= 0)
                    {
                        AnimNew();
                        AnimOriginSet(&gQavOriginTypes[i], TRUE);
                        break;
                    }
                    continue;
                default:
                    Quit();
                    return;
            }
            break;
        }
    }

    gTileView.bglayers++;
    xmpSetEditMode(0x01);

    artedInit();    // switch to art editing mode as well and don't allow to disable it
    gArtEd.mode     = kArtEdModeBatch;
    gArtEd.modeMin  = kArtEdModeBatch;
    gArtEd.modeMax  = kArtEdModeTile2D;

    toolGetResTableValues();
    toolSetOrigin(&origin, xdim >> 1, ydim >> 1);

    mouse.Init(&gMousePrefs); mouse.wheelDelay = 12;
    mouse.VelocitySet(5, 5, false);

    edit3d = (gMapLoaded && cursectnum >= 0 && screenMode == 200);
    gScreen.msg[0].time = 0;
    horiz = 100; // there is no mouse look because mouse is busy

    ProcessLoop();
    Quit();

    gTileView.bglayers--;
    gScreen.msg[0].time = 0;
    artedUninit();

    if (screenMode == 200)      xmpSetEditMode(0x01);
    else                        xmpSetEditMode(0x00);
}


void QAVEDIT::ProcessLoop()
{
    #define kMar 10
    QFONT* pFont;
    int x = kMar, y = kMar, dx, dy;
    int i, j, len, keytimer = 0;

    layer = 0;
    frame = 0;

    while( 1 )
    {
        pFrame = FrameGet(); pLayer = LayerGet(); pSound = SoundGet();
        pFont = gTool.pFont; dx = x, dy = y;
        h = (totalclock & 32) ? 8 : 0;
        viewplu = viewshade = 0;
        updateClocks();

        // screen drawing
        //////////////////

        if (!edit3d)
        {
            gfxSetColor(gStdColor[28]);
            gfxFillBox(0, 0, xdim, ydim);
            toolDrawPixels(gTool.hudPixels, gStdColor[29]);
        }
        else
        {
            processMove();
            processDrawRooms();
            if (playing && cursectnum >= 0)
            {
                sectortype* pSect = &sector[cursectnum];
                viewshade = pSect->floorshade;
                if (pSect->extra > 0 && xsector[pSect->extra].coloredLights)
                    viewplu = sector[cursectnum].floorpal;
            }
        }

        j = ydim / 3;
        toolDrawCenter(&origin, gStdColor[20], j, j, gTool.centerPixels);

        if (playing)
        {
            time.factor = 16;
            if (ctrl) time.factor--;
            if (alt)  time.factor--;

            time.ticks += gFrameTicks<<time.factor;
            if (time.ticks>>16 >= pQav->duration)
            {
                if (playing < 2) AnimStopPlaying(FALSE);
                else time.ticks -= (pQav->duration<<16);
            }

            i = time.ticks>>16;
            FrameDraw(pFrame);
            frame = ClipRange(i/pQav->ticksPerFrame, 0, pQav->nFrames-1);

            // sound
            if (pSound->id)
            {
                time.count -= gFrameTicks;
                while (time.count < 0)
                {
                    playSound(pSound->id+Random(pSound->range), 0);
                    time.count += pQav->ticksPerFrame;
                }
            }
        }
        else
        {
            FrameDraw(pFrame);
        }

        toolDrawCenter(&origin, gStdColor[20], 6, 6, 1);

        sprintf(buffer, "TICKS PER FRAME: %d", pQav->ticksPerFrame);
        len = gfxGetTextLen(buffer, pFont);
        gfxPrinTextShadow(xdim-kMar-len, dy, gStdColor[kColorLightCyan], buffer, pFont);
        //printext2(xdim-kMar-len, dy, gStdColor[kColorLightCyan], buffer, &vFonts[7]);

        sprintf(buffer, "FRAME #%d OF %d", frame+1, ClipLow(pQav->nFrames, 1));
        gfxPrinTextShadow(dx, dy, gStdColor[kColorYellow], buffer, pFont);
        dx += ClipLow(gfxGetTextLen(buffer, pFont) + pFont->width, 90);

        if (!playing)
        {
            i = 0;
            i += sprintf(buffer, "TRIGGER: %d / SOUND: %d", pFrame->nCallbackId, pSound->id);
            if (pSound->range)
                i+=sprintf(&buffer[i], "+%-02d", pSound->range);

            i+=sprintf(&buffer[i], " / PRIORITY: %d", pSound->priority);
            gfxPrinTextShadow(dx, dy, gStdColor[16], buffer, pFont);
            dy+=pFont->height+2;

            dx = x; i = 0;
            i += sprintf(buffer, "LAYER #%d", layer+1);
            if (pLayer->picnum > 0)
            {
                i+=sprintf(&buffer[i], " OF %d", kMaxLayers);
                gfxPrinTextShadow(dx, dy, gStdColor[kColorLightMagenta], buffer, pFont);
                dx += ClipLow(gfxGetTextLen(buffer, pFont) + pFont->width, 90);

                sprintf(buffer, "TILE: %05d / SHADE: %+04d / PALETTE: %03d / ZOOM: %03d%% / ANGLE: %04d / X: %04d / Y: %04d",
                pLayer->picnum, pLayer->shade, pLayer->palnum,
                (pLayer->z*100)/0x10000, pLayer->angle, pLayer->x,
                pLayer->y);

                gfxPrinTextShadow(dx, dy, gStdColor[16], buffer, pFont);
            }
            else
            {
                i+=sprintf(&buffer[i], " IS EMPTY. SELECT PICTURE TO CREATE.");
                gfxPrinTextShadow(dx, dy, gStdColor[kColorLightRed], buffer, pFont);
            }
        }

        dy+=pFont->height + 1;
        gfxSetColor(gStdColor[kColorBrown]);
        gfxHLine(dy+0, x, xdim-kMar);
        gfxHLine(dy+1, x, xdim-kMar);
        dx = x; dy+=5;

        pFont = qFonts[1];
        if (playing && (viewshade || viewplu))
        {
            i = 0;
            i+=sprintf(&buffer[i], "VIEW");
            if (viewshade) i+=sprintf(&buffer[i], "  SHADE: %+d", viewshade);
            if (viewplu)   i+=sprintf(&buffer[i], "  PALETTE: %d", viewplu);

            len = gfxGetTextLen(buffer, pFont);
            gfxPrinTextShadow(dx, dy, gStdColor[kColorRed ^ h], buffer, pFont);
        }

        if (!newQav)
        {
            i = buffer2[0] = 0;
            getFilename(gPaths.qavs, buffer2);
            if (buffer2[0])
            {
                sprintf(buffer, "%s - RFF ID: %d", buffer2, rffID);
                len = gfxGetTextLen(strupr(buffer), pFont);
                gfxPrinTextShadow((xdim>>1)-(len>>1), dy, gStdColor[16], buffer, pFont);
            }
        }

        i = frame*pQav->ticksPerFrame; j = (ClipLow(pQav->nFrames-1, 0))*pQav->ticksPerFrame;
        sprintf(buffer, "TIME: %02d:%03d / %02d:%03d",i/120, i%120, j/120, j%120);
        len = gfxGetTextLen(buffer, pFont);
        gfxPrinTextShadow(xdim-len-kMar, dy, gStdColor[16], buffer, pFont);

        dy += pFont->height+1;
        for (j = 0; j < LENGTH(gQavOriginTypes); j++)
        {
            if (pQav->x != gQavOriginTypes[j].x) continue;
            else if (pQav->y != gQavOriginTypes[j].y) continue;
            sprintf(buffer, gQavOriginNames[j]);
            break;
        }

        if (j >= LENGTH(gQavOriginTypes))
            sprintf(buffer, gQavOriginNames[kQavOriginCustom]);

        len = gfxGetTextLen(strupr(buffer), pFont);
        gfxPrinTextShadow(xdim-len-kMar, dy, gStdColor[kColorLightBlue], buffer, pFont);


        pFont = qFonts[0];
        toolDisplayMessage(gStdColor[kColorMagenta ^ h], kMar, ydim-kMar-pFont->height, pFont);
        showframe();


        handleevents();
        keyGetHelper(&key, &ctrl, &shift, &alt);
        mouse.Read();

        // GLOBAL keys
        //////////////////
        switch(key) {
            case KEY_ESC:
                if (!playing) break;
                else AnimStopPlaying(TRUE);
                continue;
            case KEY_PLUS:
            case KEY_MINUS:
                i = (key == KEY_MINUS) ? -1 : 1;
                pQav->ticksPerFrame = ClipLow(pQav->ticksPerFrame + i, 1);
                AnimDurationSet();
                break;
            case KEY_SPACE:
                if (playing)    AnimStopPlaying(FALSE);
                else if (shift) playing = 1;
                else            playing = 2;
                AnimDurationSet();
                if (pQav->duration <= 0) playing = 0;
                break;
            case KEY_PADENTER:
                edit3d = !edit3d;
                if (edit3d)
                {
                    horiz = 100;
                    if (cursectnum < 0 || !gMapLoaded)
                    {
                        Alert("3D mode unavailable.");
                        edit3d = 0;
                    }
                }
                break;
        }

        if (playing)
            continue;

        //// GLOBAL editing
        ////////////////////////////
        if (mouse.wheel)
        {
            if (mouse.wheel > 0)
            {
                if (!alt)
                {
                    if (layer > 0) key = KEY_PAGEDN;
                    else
                    {
                        key = KEY_PAGEUP;
                        layer = kMaxLayers - 1;
                    }
                }
                else if (frame > 0) key = KEY_1;
                else
                {
                    key = KEY_2;
                    frame = pQav->nFrames - 1;
                }
            }
            else
            {
                if (!alt)
                {
                    if (layer < kMaxLayers-1) key = KEY_PAGEUP;
                    else
                    {
                        key = KEY_PAGEDN;
                        layer = 0;

                    }
                }
                else if (frame < pQav->nFrames-1) key = KEY_2;
                else
                {
                    key = KEY_1;
                    frame = 0;
                }
            }

            pLayer = LayerGet();
            pFrame = FrameGet();
            pSound = SoundGet();
        }

        switch (key) {
            case KEY_TAB:
                scrSetMessage("Frame copied in clipboard.");
                memcpy(&cpbrd.frame, pFrame, sizeof(FRAMEINFO));
                cpbrd.layer = layer;
                cpbrd.ok = 1;
                break;
            case KEY_ENTER:
                buffer[0] = '\0';
                if (!cpbrd.ok)
                {
                    scrSetMessage("Clipboard is empty!");
                }
                else if (alt)
                {
                    memcpy(pFrame, &cpbrd.frame, sizeof(FRAMEINFO));
                    sprintf(buffer, "Frame");
                }
                else if (ctrl)
                {
                    i = (shift) ? cpbrd.layer : layer;
                    memcpy(pLayer, &cpbrd.frame.tiles[i], sizeof(TILE_FRAME));
                    sprintf(buffer, "Layer");
                }
                else
                {
                    i = (shift) ? cpbrd.layer : layer;
                    if (cpbrd.frame.tiles[i].picnum > 0)
                    {
                        LayerSetTile(frame, layer, cpbrd.frame.tiles[i].picnum);
                        pLayer->picnum = cpbrd.frame.tiles[i].picnum;
                        sprintf(buffer, "Tile");
                    }
                }

                if (buffer[0])
                    scrSetMessage("%s pasted from clipboard.", buffer);
                break;
            case KEY_F1:
                if (!Confirm("Start new animation now?")) break;
                else if ((i = AnimOriginSelect()) >= 0)
                {
                    AnimNew();
                    AnimOriginSet(&gQavOriginTypes[i], TRUE);
                    scrSetMessage("New animation started.");
                }
                break;
            case KEY_ESC:
                if (!gArtEd.asksave && CRC == crc32once((unsigned char*)pQav, kAnimSize))
                {
                    if (!Confirm("Quit now?")) break;
                    return;
                }
                else if ((i = DlgSaveChanges("Save changes?", gArtEd.asksave)) == -1) break;
                else if (i == 0) return;
                // no break
            case KEY_F2:
                if ((ctrl || !fileExists(gPaths.qavs)) && !toolSaveAs(gPaths.qavs, kQavExt)) break;
                else if (!AnimSave(gPaths.qavs)) Alert("Failed to save file \"%s\"", gPaths.qavs);
                else
                {
                    rffID = -1;
                    
                    if (key == KEY_ESC)
                    {
                        if (i > 1)
                            artedSaveChanges();
                        
                        // quitting now
                        return;
                    }
                    
                    if (gArtEd.asksave && Confirm("Save ART changes?"))
                        artedSaveChanges();
                    
                    scrSetMessage("Saved to \"%s\"", gPaths.qavs);
                }
                break;
            case KEY_F11: // load previous existing fileID in rff
            case KEY_F12: // load next existing fileID in rff
            case KEY_F3:  // call load dialog
                i = -1;
                if (key != KEY_F3)
                {
                    i = getClosestId(ClipRange(rffID, 0, 65534), 65534, gExtNames[kQav], (BOOL)(key == KEY_F12));
                    sprintf(gPaths.qavs, "%d.%s", i, gExtNames[kQav]);
                }
                if (i < 0 && !toolLoadAs(gPaths.qavs, kQavExt, "Load QAV")) break;
                else if (!AnimLoad(gPaths.qavs)) Alert("Failed to load file \"%s\"!", gPaths.qavs);
                else
                {
                    scrSetMessage("File \"%s\" loaded from %s", gPaths.qavs, (rffID >= 0) ? gPaths.bloodRFF : "disk");
                    frame = layer = 0;
                    pQav->Preload();
                }
                break;
            case KEY_F4: // import animation
                AnimImportDlg();
                continue;
            case KEY_DELETE:
                buffer[0] = '\0';
                if (ctrl)
                {
                    LayerDelete(frame, layer);
                    sprintf(buffer, "Layer");
                }
                else if (alt)
                {
                    if (pQav->nFrames > 0)
                    {
                        FrameDelete(frame);
                        if (frame >= pQav->nFrames)
                            frame = ClipLow(pQav->nFrames-1, 0);

                        sprintf(buffer, "Frame");
                    }
                }
                else if (pLayer->picnum > 0)
                {
                    pLayer->picnum = 0;
                    sprintf(buffer, "Tile of layer %d", layer+1);
                }

                if (buffer[0])
                    scrSetMessage("%s deleted.", buffer);
                continue;
            case KEY_INSERT:
                buffer[0] = '\0';
                if (alt)
                {
                    if (frame < kMaxFrames)
                    {
                        FrameInsert(frame, FALSE);
                        frame = ClipHigh(frame+1, pQav->nFrames-1);
                        sprintf(buffer, "Frame");
                    }
                    else
                    {
                        Alert("Max frames reached!");
                        break;
                    }
                }
                else if (pLayer->picnum <= 0)
                {
                    if (frame > 0)
                    {
                        pLayer->picnum = pQav->frames[frame-1].tiles[layer].picnum;
                        if (pLayer->z <= 0)
                            pLayer->z = 0x10000;
                    }

                    if (pLayer->picnum <= 0)
                        LayerSetTile(frame, layer);

                    if (pLayer->picnum > 0)
                        sprintf(buffer, "Tile of layer %d", layer+1);
                }

                if (buffer[0])
                    scrSetMessage("%s inserted.", buffer);
                continue;
            case KEY_1:
            case KEY_2:
            case KEY_G:
            case KEY_HOME:
            case KEY_END:
                if (pQav->nFrames <= 0) break;
                else if (key == KEY_HOME)   j = 0;
                else if (key == KEY_END)    j = pQav->nFrames - 1;
                else if (key == KEY_G)      j = GetNumberBox("Go to frame", frame, frame);
                else if (key == KEY_1)      j = frame - 1;
                else if (key == KEY_2)      j = frame + 1;
                frame = ClipRange(j, 0, pQav->nFrames - 1);
                break;
            case KEY_S:
                if (ctrl && pSound->id) pSound->range = GetNumberBox("Random Sound Range", pSound->range, pSound->range);
                else pSound->id = GetNumberBox("Frame sound ID", pSound->id, pSound->id);
                pSound->range = ClipRange(pSound->range, 0, 15);
                pSound->id    = ClipLow(pSound->id, 0);
                break;
            case KEY_R:
                pSound->priority = GetNumberBox("Sound priority", pSound->priority, pSound->priority);
                break;
            case KEY_F:
                pFrame->nCallbackId = GetNumberBox("Frame trigger", pFrame->nCallbackId, pFrame->nCallbackId);
                break;
            case KEY_V:
                LayerSetTile(frame, layer);
                break;
            case KEY_F8:
                if ((i = AnimOriginSelect()) < 0) break;
                else if (pQav->nFrames > 0) j = Confirm("Adjust frames?");
                AnimOriginSet(&gQavOriginTypes[i], j);
                scrSetMessage("QAV Origin changed to %s", gQavOriginNames[i]);
                break;
            case KEY_F9: // reverse frames order
            {
                FRAMEINFO tmp;
                for (i = 0; i < pQav->nFrames >> 1; i++)
                {
                    tmp = pQav->frames[i];
                    pQav->frames[i] = pQav->frames[pQav->nFrames - i - 1];
                    pQav->frames[pQav->nFrames - i - 1] = tmp;
                }
                scrSetMessage("Reverse frames order.");
                break;
            }
            case KEY_F10:
                if (playing) break;
                else if (ctrl) playSound(pSound->id, 2);
                else playSound(pSound->id+Random(pSound->range), 2);
                break;
            case KEY_PAGEUP:
            case KEY_PAGEDN:
                if ((j = kneg(1, true)) > 0 && layer >= kMaxLayers - 1) break;
                else if (j < 0 && layer <= 0) break;
                else if (pLayer->picnum > 0)
                {
                    buffer[0] = '\0';
                    if (ctrl)
                    {
                        for (i = 0; i < pQav->nFrames; i++)
                        {
                            TILE_FRAME tmp = pQav->frames[i].tiles[layer];
                            pQav->frames[i].tiles[layer] = pQav->frames[i].tiles[layer + j];
                            pQav->frames[i].tiles[layer + j] = tmp;
                        }
                        sprintf(buffer, "layer in all frames");
                    }
                    else if (shift)
                    {
                        TILE_FRAME tmp = *pLayer;
                        *pLayer = pFrame->tiles[layer + j];
                        pFrame->tiles[layer + j] = tmp;
                        sprintf(buffer, "layer");
                    }

                    if (buffer[0])
                        scrSetMessage("%s %s", (j < 0) ? "lower" : "raise", buffer);
                }
                layer+=j;
                break;
        }


        //// LAYER editing
        ////////////////////////////
        if (pLayer->picnum <= 0) continue;
        else if (mouse.hold & 1)
        {
            if (mouse.dX2) pLayer->x += mouse.dX2;
            if (mouse.dY2) pLayer->y += mouse.dY2;
            LayerClip(frame, layer);
            continue;
        }
        else if (keytimer < totalclock)
        {
            i = 0;
            keytimer = totalclock + 6;
            BOOL UP = ((!edit3d && keystatus[KEY_UP])    || keystatus[KEY_PADUP]);
            BOOL DW = ((!edit3d && keystatus[KEY_DOWN])  || keystatus[KEY_PADDOWN]);
            BOOL LT = ((!edit3d && keystatus[KEY_LEFT])  || keystatus[KEY_PADLEFT]);
            BOOL RT = ((!edit3d && keystatus[KEY_RIGHT]) || keystatus[KEY_PADRIGHT]);
            if (UP | DW)
            {
                if (alt) pLayer->y = GetNumberBox("Layer y-coordinate", pLayer->y, pLayer->y);
                else pLayer->y += (UP) ? -1 : 1;
                LayerClip(frame, layer);
                i = 1;
            }

            if (LT | RT)
            {
                if (alt) pLayer->x = GetNumberBox("Layer x-coordinate", pLayer->x, pLayer->x);
                else pLayer->x += (LT) ? -1 : 1;
                LayerClip(frame, layer);
                i = 1;
            }

            if (i)
                continue;
        }

        switch (key) {
            case KEY_P:
                i = pLayer->palnum;
                if (shift & 0x01)       i = getClosestId(i, kPluMax - 1, "PLU", TRUE);
                else if (shift & 0x02)  i = getClosestId(i, kPluMax - 1, "PLU", FALSE);
                else if (alt)           i = pluPickAdvanced(pLayer->picnum, pLayer->shade, pLayer->palnum, "Palookup");
                pLayer->palnum  = ClipRange(i, 0, kPluMax - 1);
                break;
            case KEY_T:
                j = 0, i = pLayer->stat;
                if (i & kRSTransluc)
                    j = (i & kRSTranslucR) ? 2 : 1;

                switch (j = IncRotate(j, 3)) {
                    case 0:
                        i &= ~kRSTransluc & ~kRSTranslucR;
                        break;
                    case 1:
                        i &= ~kRSTranslucR;
                        i |=  kRSTransluc;
                        break;
                    case 2:
                        i |= (kRSTransluc | kRSTranslucR);
                        break;
                }
                pLayer->stat = i;
                break;
            case KEY_X:
                pLayer->stat ^= 0x0100;
                break;
            case KEY_Y:
                pLayer->stat ^= kRSYFlip;
                break;
            case KEY_COMMA:
            case KEY_PERIOD:
                i = (shift) ? ((ctrl) ? 1 : 4) : 128;
                if (key == KEY_PERIOD) pLayer->angle = IncNext(pLayer->angle, i);
                else pLayer->angle = DecNext(pLayer->angle, i);
                pLayer->angle = pLayer->angle & kAngMask;
                break;
            case KEY_PADSLASH:
            case KEY_PADSTAR:
                i = (shift) ? ((ctrl) ? 0x050: 0x100) : 0x1000;
                pLayer->z = ClipLow(pLayer->z + kneg(i), i);
                break;
            case KEY_PAD0:
            case KEY_PADMINUS:
            case KEY_PADPLUS:
                i = pLayer->shade;
                if (key == KEY_PAD0) i = 0;
                else if (alt) i = GetNumberBox("Shade", i, i);
                else if (key == KEY_PADMINUS) i++;
                else i--;
                pLayer->shade = ClipRange(i, -127, 128);
                break;
        }
    }
}

BOOL QAVEDIT::AnimNew()
{
    int i;
    if (!pQav && (pQav = (QAV*)malloc(kAnimSize)) == NULL)
        return FALSE;

    memset(pQav, 0, kAnimSize);
    pQav->x = gQavOriginTypes[kQavOriginPlayer].x;
    pQav->y = gQavOriginTypes[kQavOriginPlayer].y;
    pQav->ticksPerFrame = 5;

    for (i = 0; i < kMaxFrames; i++)
        FrameClean(&pQav->frames[i]);

    CRC    = crc32once((unsigned char*)pQav, kAnimSize);
    frame  = 0; layer = 0;
    newQav = 1;
    return TRUE;
}

int QAVEDIT::AnimOriginSelect()
{
    return showButtons(gQavOriginNames, LENGTH(gQavOriginNames) - 1, "Select origin") - mrUser;
}

void QAVEDIT::AnimStopPlaying(BOOL reset)
{
    playing = 0;
    if (reset)
    {
        frame = 0;
        memset(&time, 0, sizeof(time));
    }
    sndKillAllSounds();
    return;
}

void QAVEDIT::Quit()
{
    if (pQav)
    {
        free(pQav);
        pQav = NULL;
    }

    if (pImportPath)
    {
        free(pImportPath);
        pImportPath = NULL;
    }

    cpbrd.ok = 0;
}

BOOL QAVEDIT::AnimLoad(char* filename, QAV** pOut)
{
    RESHANDLE pFile = NULL;
    int hFile, i, nSize; QAV* tmp; BOOL retn = FALSE;
    BOOL edit = (pOut == NULL);

    if ((i = fileExists(filename, &pFile)) <= 0) return FALSE;
    else if ((tmp = (QAV*)malloc(kAnimSize)) == NULL) return FALSE;
    else memset(tmp, 0, kAnimSize);

    // first try load from the disk
    if ((i & 0x01) && ((hFile = open(filename, O_RDONLY|O_BINARY, S_IWRITE|S_IREAD)) >= 0))
    {
        nSize = ClipHigh(filelength(hFile), kAnimSize);
        read(hFile, tmp, nSize);
        close(hFile);
        if (edit)
            rffID = -1;
    }
    else if (pFile != NULL) // rff otherwise
    {
        QAV* pRFFQav = (QAV*)gSysRes.Load(pFile);
        nSize = ClipHigh(gSysRes.Size(pFile), kAnimSize);
        memcpy(tmp, pRFFQav, nSize);
        if (edit)
        {
            sprintf(gPaths.qavs, pFile->name);
            rffID = pFile->id;
        }
    }

    if ((retn = (strcmp(tmp->sign, kQavSig) == 0 && tmp->version == kQavVersion)) != FALSE)
    {
        if (edit)
        {
            memcpy(pQav, tmp, nSize);
            CRC = crc32once((unsigned char*)pQav, kAnimSize); // save to compare changes
            newQav = 0;
        }
        else
        {
            memcpy(*pOut, tmp, nSize);
        }
    }

    free(tmp);
    return retn;
}

void QAVEDIT::AnimImportDlg()
{
    int i, j, nType, nTTimes, nCTimes, nIFrames, nTFrames; QAV* pImport; BOOL replace;
    if ((pImport = (QAV*)malloc(kAnimSize)) == NULL) return;
    else if (!pImportPath)
    {
        if ((pImportPath = (char*)malloc(_MAX_PATH)) == NULL) return;
        else sprintf(pImportPath, gPaths.qavs);
    }

    while( 1 )
    {
        if (!toolLoadAs(pImportPath, kQavExt, "Import QAV")) break;
        else if (!AnimLoad(pImportPath, &pImport))
        {
            Alert("Could not load file \"%s\".", gPaths.qavs);
            continue;
        }

        nIFrames = pImport->nFrames;
        nTFrames = 0;
        nTTimes = 1;

        while( 1 )
        {
            sprintf(buffer, "Import %d frames", nIFrames);
            if (pQav->nFrames <= 0) nType = 0;
            else if ((nType = showButtons(gImportOptions, LENGTH(gImportOptions), buffer) - mrUser) < 0)
                break;

            sprintf(buffer, "How many times? (%d ... %d)", 1, kImportTimes);
            if ((i = GetNumberBox(buffer, nTTimes, -1)) <= 0)
                continue;

            nCTimes = nTTimes = ClipHigh(i, kImportTimes);

            while (nCTimes)
            {
                if (nType == 0)
                {
                    j = pQav->nFrames;
                    for (i = 0; i < pImport->nFrames && j < kMaxFrames; j++, nTFrames++)
                        pQav->frames[j] = pImport->frames[i++];

                    pQav->nFrames = j;
                }
                else
                {
                    if (nCTimes == nTTimes)
                    {
                        sprintf(buffer, "Frame # (%d ... %d)", frame, pQav->nFrames);
                        if ((j = GetNumberBox(buffer, frame, -1)) < 0) continue;
                        else if ((j = ClipHigh(j, pQav->nFrames)) <= pQav->nFrames)
                            replace = Confirm("Replace existing frames?");
                    }

                    for (i = 0; i < pImport->nFrames && j < kMaxFrames; j++, nTFrames++)
                    {
                        if (!replace || j >= pQav->nFrames)
                            FrameInsert(j, FALSE);

                        pQav->frames[j] = pImport->frames[i++];
                    }
                }

                if (j >= kMaxFrames)
                {
                    Alert("Max frames reached!");
                    break;
                }

                nCTimes--;
            }

            Alert("%d frames imported in total, %d times passed.", nTFrames, nTTimes-nCTimes);
            break;
        }

        if (i >= 0)
            break;
    }

    free(pImport);
    return;
}

BOOL QAVEDIT::AnimSave(char* filename)
{
    int hFile;
    char name[_MAX_PATH];
    sprintf(name, filename);
    ChangeExtension(name, getExt(kQav));

    if ((hFile = open(name, O_CREAT|O_WRONLY|O_BINARY|O_TRUNC, S_IWRITE|S_IREAD)) < 0)
        return FALSE;

    pQav->version = kQavVersion;
    if (pQav->nFrames <= 0)
    {
        pQav->nFrames++;
        FrameClean(&pQav->frames[0]);
    }
    memcpy(pQav->sign, kQavSig, sizeof(pQav->sign)); AnimDurationSet();
    write(hFile, pQav, sizeof(QAV) + (pQav->nFrames*sizeof(FRAMEINFO)));

    // update crc
    CRC = crc32once((unsigned char*)pQav, kAnimSize);
    newQav = 0;
    return TRUE;
}


void QAVEDIT::FrameInsert(int nFrame, bool after)
{
    int i, j;

    pQav->nFrames++;
    if (after)
    {
        j = nFrame + 1;
        for (i = pQav->nFrames - 1; i > j; i--)
            pQav->frames[i] = pQav->frames[i - 1];

        FrameClean(&pQav->frames[i]);
    }
    else
    {
        FRAMEINFO* pFrame = &pQav->frames[nFrame];
        memmove(pFrame+1, pFrame, (pQav->nFrames-nFrame)*sizeof(FRAMEINFO));
    }
}

void QAVEDIT::FrameDelete(int nFrame)
{
    if (pQav->nFrames > 0)
    {
        FRAMEINFO* pFrame = &pQav->frames[nFrame];
        memmove(pFrame, pFrame+1, (pQav->nFrames-nFrame)*sizeof(FRAMEINFO));
        FrameClean(&pQav->frames[pQav->nFrames]);
        pQav->nFrames--;
    }
}

void QAVEDIT::FrameClean(FRAMEINFO* pFrame)
{
    int i;
    memset(pFrame, 0, sizeof(FRAMEINFO));
    pFrame->nCallbackId = pFrame->sound.id = -1;
    for (i = 0; i < kMaxLayers; i++)
        LayerClean(frame, i);
}

void QAVEDIT::FrameDraw(FRAMEINFO* pFrame)
{
    int i;
    for (i = 0; i < kMaxLayers; i++)
    {
        TILE_FRAME* pLayer = &pFrame->tiles[i];
        if (pLayer->picnum > 0)
        {
            DrawFrame(pQav->x, pQav->y, pLayer, 0x02, viewshade, viewplu);
            if (!playing && layer == i)
                LayerHighlight(frame, i);
        }
    }
}

void QAVEDIT::LayerDelete(int nFrame, int nLayer)
{
    int i, j;
    for (i = 0; i < pQav->nFrames; i++)
    {
        for (j = nLayer; j < kMaxLayers-1; j++)
            pQav->frames[i].tiles[j] = pQav->frames[i].tiles[j+1];

        LayerClean(i, j);
    }
}

void QAVEDIT::LayerClean(int nFrame, int nLayer)
{
    TILE_FRAME* pLayer = LayerGet(nFrame, nLayer);
    memset(pLayer, 0, sizeof(TILE_FRAME));
}


void QAVEDIT::LayerHighlight(int nFrame, int nLayer)
{
    int xoffs, yoffs, t, x[4], y[4];
    if (pLayer->stat & kRSCorner) xoffs = yoffs = 0;
    else
    {
        xoffs = mulscale16(panm[pLayer->picnum].xcenter, pLayer->z);
        yoffs = mulscale16(panm[pLayer->picnum].ycenter, pLayer->z);
    }
    
    GetRotateSpriteExtents
    (
        (pQav->x - xoffs) + pLayer->x,
        (pQav->y - yoffs) + pLayer->y,
        pLayer->z,
        pLayer->angle,
        pLayer->picnum,
        pLayer->stat | kRSScale,
        xdim-1, ydim-1,
        x,
        y
    );
    
    gfxSetColor(fade(256));
    t = drawlinepat, drawlinepat = kPatDotted;
    gfxLine(x[0], y[0], x[1], y[1]); gfxLine(x[1], y[1], x[2], y[2]);
    gfxLine(x[2], y[2], x[3], y[3]); gfxLine(x[3], y[3], x[0], y[0]);
    drawlinepat = t;
}

void QAVEDIT::LayerClip(int nFrame, int nLayer)
{
    int xOffset, yOffset;
    TILE_FRAME* pLayer = LayerGet(nFrame, nLayer);

    if (!pLayer || pLayer->picnum < 0) return;
    else if (pLayer->stat & kRSCorner) xOffset = yOffset = 0;
    else
    {
        xOffset = panm[pLayer->picnum].xcenter + tilesizx[pLayer->picnum] / 2;
        yOffset = panm[pLayer->picnum].ycenter + tilesizy[pLayer->picnum] / 2;
    }

    xOffset -= pQav->x; yOffset -= pQav->y;
    pLayer->x = ClipRange(pLayer->x, xOffset-tilesizx[pLayer->picnum]+1, 319 + xOffset);
    pLayer->y = ClipRange(pLayer->y, yOffset-tilesizy[pLayer->picnum]+1, 199 + yOffset);

}

void QAVEDIT::AnimOriginSet(POINT2D* pOrigin, BOOL adjust)
{

    int i, j, dx, dy;
    int x = pOrigin->x, y = pOrigin->y;

    if (adjust)
    {
        x = ClipRange(x, 0, 319);
        y = ClipRange(y, 0, 199);
        dx = x - pQav->x;
        dy = y - pQav->y;
        for (i = 0; i < pQav->nFrames; i++)
        {
            for (j = 0; j < kMaxLayers; j++)
            {
                pQav->frames[i].tiles[j].x -= dx;
                pQav->frames[i].tiles[j].y -= dy;
            }
        }
        pQav->x = x;
        pQav->y = y;
    }
    else
    {
        pQav->x = ClipRange(x, 0, 319);
        pQav->y = ClipRange(y, 0, 199);
        for (i = 0; i < pQav->nFrames; i++)
        {
            for (j = 0; j < kMaxLayers; j++)
                LayerClip(i, j);
        }
    }
}

void QAVEDIT::LayerSetTile(int nFrame, int nLayer, int nTile)
{
    int i;

    TILE_FRAME* pLayer = LayerGet(nFrame, nLayer);
    if (nTile > 0 || (nTile = tilePick(pLayer->picnum, -1, OBJ_ALL)) > 0)
    {
        if (pLayer->z <= 0) pLayer->z = 0x10000;
        pLayer->picnum = nTile;
    }
}

FRAMEINFO* QAVEDIT::FrameGet(int nFrame)
{
    if (nFrame < 0) nFrame = frame;
    return &pQav->frames[nFrame];
}

SOUNDINFO* QAVEDIT::SoundGet(int nFrame)
{
    if (nFrame < 0) nFrame = frame;
    return &pQav->frames[nFrame].sound;
}

TILE_FRAME* QAVEDIT::LayerGet(int nFrame, int nLayer)
{
    if (nFrame < 0) nFrame = frame;
    if (nLayer < 0) nLayer = layer;
    return &pQav->frames[nFrame].tiles[nLayer];
}