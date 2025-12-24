/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025: Originally written by NoOne.
// Implementation of SEQ animation files editor (SEQEDIT).
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

#include "tile.h"
#include "xmpsnd.h"
#include "xmptools.h"
#include "xmpseqed.h"
#include "sectorfx.h"
#include "xmpexplo.h"

#define MSG_TIME        80
#define kMaxFrames      256
#define kAnimSize       sizeof(Seq) + (sizeof(SEQFRAME)*kMaxFrames)

SEQEDIT gSeqed;

static MAPEDIT_HUD_STATUS statViewZ;
static MAPEDIT_HUD_STATUS statViewZoom;
static MAPEDIT_HUD_STATUS statViewShade;
static MAPEDIT_HUD_STATUS statViewSurface;
static MAPEDIT_HUD gSeqedHud;

static void dlgSeqToDialog(DIALOG_HANDLER* pHandle, Seq* pSeq, SEQFRAME* pFrame);
static void dlgDialogToSeq(DIALOG_HANDLER* pHandle, Seq* pSeq, SEQFRAME* pFrame);

static char helperToggleSeqFlags(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key);
static char helperAuditSound(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key);

static char* pasteDirection[] =
{
    "&Current frame only",
    "All &next frames",
    "All &previous frames",
    "&All frames",
};

static char* spawnMenu[] =
{
    "&Create new sprite",
    "&Edit SEQ for current object",
    "&Quit",
};

static char* viewZNames[] =
{
    "Floor",
    "Middle",
    "Ceiling",
    "Original",
};

static CHECKBOX_LIST clipboardMenu[] =
{
    {0,     "&Tile"},
    {1,     "&X-repeat"},
    {1,     "&Y-repeat"},
    {1,     "&Shade"},
    {1,     "&Palette"},
    {0,     "&Flags"},
};

static enum
{
    kSeqDlgSpeed       = 1,
    kSeqDlgFrames,
    kSeqDlgSound,
    kSeqDlgRemove,
    kSeqDlgLoop,
    kSeqDlgFramePic,
    kSeqDlgFramePal,
    kSeqDlgFrameXSiz,
    kSeqDlgFrameYSiz,
    kSeqDlgFrameShade,
    kSeqDlgSndRange,
    kSeqDlgFrameBLK,
    kSeqDlgFrameHSC,
    kSeqDlgFrameAIM,
    kSeqDlgFrameTRG,
    kSeqDlgFramePSH,
    kSeqDlgFrameSMK,
    kSeqDlgFrameTR1,
    kSeqDlgFrameTR2,
    kSeqDlgFrameFLX,
    kSeqDlgFrameFLY,
    kSeqDlgFrameINV,
    kSeqDlgFrameSND,
    kSeqDlgFrameSRF,
};

DIALOG_ITEM dlgSeqedit[] =
{
    { NO,   1,      0,  0,  0,                      HEADER,     "Animation         " },
    { NO,   1,      0,  1,  kSeqDlgSpeed,           NUMBER,     "Delay........: %-5d",              0,     32767,           NULL,   NULL},
    { NO,   1,      0,  2,  kSeqDlgFrames,          NUMBER,     "Frames.......: %-3d",              0,     kMaxFrames,      NULL,   NULL},
    { NO,   1,      0,  3,  kSeqDlgSound,           NUMBER,     "Sound........: %-5d",              0,     32767,           NULL,   helperAuditSound},

    { NO,   1,      0,  6,  0,                      HEADER,     "Playback done" },
    { NO,   1,      0,  7,  kSeqDlgRemove,          CHECKBOX,   "Delete",                           0,      1,              NULL, helperToggleSeqFlags},
    { NO,   1,      0,  8,  kSeqDlgLoop,            CHECKBOX,   "Loop",                             0,      1,              NULL, helperToggleSeqFlags},

    { NO,   1,      22,  0,  0,                     HEADER,     "Frame properties   " },
    { NO,   1,      22,  1,  kSeqDlgFramePic,       NUMBER,     "Picnum.......: %-5d",              0,      kMaxTiles - 1,  NULL,   NULL},
    { NO,   1,      22,  2,  kSeqDlgFramePal,       NUMBER,     "Palookup.....: %-2d",              0,      127,            NULL,   NULL},
    { NO,   1,      22,  3,  kSeqDlgFrameXSiz,      NUMBER,     "X-repeat.....: %-3d",              0,      255,            NULL,   NULL},
    { NO,   1,      22,  4,  kSeqDlgFrameYSiz,      NUMBER,     "Y-repeat.....: %-3d",              0,      255,            NULL,   NULL},
    { NO,   1,      22,  5,  kSeqDlgFrameShade,     NUMBER,     "Shade........: %-+4d",             -128,   127,            NULL,   NULL},
    { NO,   1,      22,  6,  kSeqDlgSndRange,       NUMBER,     "Sound range..: %-2d",              0,      15,             NULL,   NULL},

    { NO,   1,      45,  0,  0,                      HEADER,     "Frame flags              " },
    { NO,   1,      45,  1,  kSeqDlgFrameBLK,        CHECKBOX,   "Blocking" },
    { NO,   1,      45,  2,  kSeqDlgFrameHSC,        CHECKBOX,   "Hitscan" },
    { NO,   1,      45,  3,  kSeqDlgFrameAIM,        CHECKBOX,   "Auto-aim" },
    { NO,   1,      45,  4,  kSeqDlgFrameTRG,        CHECKBOX,   "Trigger" },

    { NO,   1,      45,  6,  kSeqDlgFramePSH,        CHECKBOX,   "Pushable" },
    { NO,   1,      45,  7,  kSeqDlgFrameSMK,        CHECKBOX,   "Smoke" },
    { NO,   1,      45,  8,  kSeqDlgFrameTR1,        CHECKBOX,   "Trans 75%",                      0,      1,              NULL, helperToggleSeqFlags},
    { NO,   1,      45,  9,  kSeqDlgFrameTR2,        CHECKBOX,   "Trans 50%",                      0,      1,              NULL, helperToggleSeqFlags},

    { NO,   1,      59,  1,  kSeqDlgFrameFLX,        CHECKBOX,   "X-flipped" },
    { NO,   1,      59,  2,  kSeqDlgFrameFLY,        CHECKBOX,   "Y-flipped" },
    { NO,   1,      59,  3,  kSeqDlgFrameINV,        CHECKBOX,   "Invisible" },
    { NO,   1,      59,  5,  kSeqDlgFrameSND,        CHECKBOX,   "Play sound" },
    { NO,   1,      59,  6,  kSeqDlgFrameSRF,        CHECKBOX,   "Surf sound" },

    { NO,   1,      0,  0,  0,  CONTROL_END },
};

void SEQEDIT::Start(char* filename)
{
    SNAPSHOT_MANAGER mapState(boardSnapshotMake, boardSnapshotLoad);
    int i, s, e, ax, ay;
    int nSpr, nSect;
    
    mapState.Make(0);

    pViewXSect  = NULL;
    obj.type    = OBJ_NONE;
    rffID       = -1;
    obj.index   = -1;
    nSect       = -1;
    edit3d      =  0;

    if (numsectors > 0 && getHighlightedObject())
    {
        if (searchstat != OBJ_SPRITE && cursectnum >= 0)
        {
            switch(i = showButtons(spawnMenu, LENGTH(spawnMenu), "Object is not an sprite...") - mrUser)
            {
                case 0:
                    if ((nSpr = InsertSprite(cursectnum, 0)) >= 0)
                    {
                        sprite[nSpr].x = posx;
                        sprite[nSpr].y = posy;
                        sprite[nSpr].z = posz;
                        sprite[nSpr].ang = (ang + kAng180) & kAngMask;

                        clampSprite(&sprite[nSpr]);

                        searchstat      = OBJ_SPRITE;
                        searchsector    = cursectnum;
                        searchwall      = searchindex = nSpr;
                        offsetPos(0, -1024, 0, ang, &posx, &posy, NULL);
                        break;
                    }
                    Alert("Unable to create new sprite!");
                    // no break
                case 1:
                    break;
                default:
                    return;
            }
        }

        switch(obj.type = searchstat)
        {
            case OBJ_FLOOR:
            case OBJ_CEILING:
                GetXSector(searchsector);
                obj.index                       = searchsector;
                nSect                           = searchsector;
                if (searchstat == OBJ_FLOOR)
                {
                    refinfo.pic                     = sector[searchsector].floorpicnum;
                    refinfo.pal                     = sector[searchsector].floorpal;
                    sector[searchsector].floorpal   = 0;
                }
                else
                {
                    refinfo.pic                     = sector[searchsector].ceilingpicnum;
                    refinfo.pal                     = sector[searchsector].ceilingpal;
                    sector[searchsector].ceilingpal = 0;
                }
                
                avePointSector(searchsector, &ax, &ay);
                ang = getangle(ax - posx, ay - posy);
                break;
            case OBJ_WALL:
            case OBJ_MASKED:
                GetXWall(searchwall);
                obj.index                       = searchwall;
                nSect                           = sectorofwall(searchwall);
                refinfo.pic                     = (obj.type == OBJ_WALL)
                                                ? wall[searchwall].picnum
                                                : wall[searchwall].overpicnum;
                refinfo.pal                     = wall[searchwall].pal;
                refinfo.xsiz                    = wall[searchwall].xrepeat;
                refinfo.ysiz                    = wall[searchwall].yrepeat;
                wall[searchwall].pal            = 0;
                
                avePointWall(searchwall, &ax, &ay);
                ang = getangle(ax - posx, ay - posy);
                break;
            case OBJ_SPRITE:
                GetXSprite(searchwall);
                obj.index                       = searchwall;
                nSect                           = sprite[searchwall].sectnum;
                refinfo.pic                     = sprite[searchwall].picnum;
                refinfo.pal                     = sprite[searchwall].pal;
                refinfo.xsiz                    = sprite[searchwall].xrepeat;
                refinfo.ysiz                    = sprite[searchwall].yrepeat;
                refinfo.z                       = sprite[searchwall].z;
                ang = getangle(sprite[searchwall].x - posx, sprite[searchwall].y - posy);
                sprite[searchwall].pal          = 0;
                break;
        }

        if (nSect >= 0)
        {
            sectortype* pSect = &sector[nSect];
            viewSurface = surfType[pSect->floorpicnum];
            viewShade   = pSect->floorshade;
            
            pSect->floorstat    |= kSectShadeFloor;
            
            pSect->floorshade    = 0;
            if (!isSkySector(nSect, OBJ_CEILING))
                pSect->ceilingshade  = 0;

            getSectorWalls(nSect, &s, &e);
            while(s <= e)
                wall[s].shade = 0, s++;

            for (s = headspritesect[nSect]; s >= 0; s = nextspritesect[s])
                sprite[s].shade = 0;

            if (pSect->extra > 0)
                dbDeleteXSector(pSect->extra);

            GetXSector(nSect);

            pViewXSect = &xsector[pSect->extra];
            pViewXSect->shadeAlways     = 1;
            pViewXSect->shadeFloor      = 1;
            pViewXSect->shadeWalls      = 1;
            pViewXSect->amplitude       = ClipLow(viewShade, 1);
            
            if (!isSkySector(nSect, OBJ_CEILING))
                pViewXSect->shadeCeiling    = 1;
            
            InitSectorFX();
            DoSectorLighting();
            edit3d = 1;
        }
    }

    AnimNew();

    if (filename)
    {
        strcpy(gPaths.seqs, filename);
        if (!AnimLoad(gPaths.seqs))
            Alert("Failed to load file \"%s\"!", gPaths.seqs), filename = NULL;
    }

    if (!filename)
    {
        while( 1 )
        {
            switch (showButtons(toolMenu, LENGTH(toolMenu), gToolNames[kToolSeqEdit].name) - mrUser)
            {
                case 1:
                    if (!browseOpen(gPaths.seqs, kSeqExt)) continue;
                    else if (AnimLoad(gPaths.seqs)) break;
                    else Alert("Failed to load file \"%s\"!", gPaths.seqs);
                    continue;
                case 0:
                    strcpy(gPaths.seqs, "newanim");
                    AnimNew();
                    break;
                default:
                    Quit();
                    return;
            }

            break;
        }
    }

    xmpSetEditMode(0x01);
    seqKillAll();
    sfxInit();

    gHovSpr  = -1;
    gHovWall = -1;
    gHovStat = OBJ_NONE;
    showinvisibility = 0;

    pHud = &gSeqedHud;
    pHud->SetView(kHudLayoutFull, gHudPrefs.fontPack);
    pHud->SetLogo("XSEQEDIT", (char*)build_date, gSysTiles.icoXmp);
    hudSetLayout(pHud, kHudLayoutFull, &gMouse);

    gTileView.bglayers++;

    artedInit();    // switch to art editing mode as well and don't allow to disable it
    gArtEd.mode     = kArtEdModeBatch;
    gArtEd.modeMin  = kArtEdModeBatch;
    gArtEd.modeMax  = kArtEdModeBatch;

    origin.x = MIDPOINT(0, xdim);
    origin.y = MIDPOINT(0, ydim-pHud->Height());

    HudUpdateStatus();
    ProcessLoop();
    Quit();

    gTileView.bglayers--;
    artedUninit();
    keyClear();

    mapState.Load(0);
}

void SEQEDIT::ProcessLoop()
{
    char quit = 0;
    int nPlayTime = 0, nKeyTime = 0;
    int nTile = 0, nStep;
    int i, j, k;
    
    nFrame = 0;

    while( 1 )
    {
        updateClocks();

        if (playing && pSeq->ticksPerFrame)
        {
            nPlayTime -= gFrameTicks;
            while(nPlayTime < 0)
            {
                nPlayTime += pSeq->ticksPerFrame;
                if (++nFrame == pSeq->nFrames)
                {
                    if (playing & 0x02)
                    {
                        nFrame = 0;
                    }
                    else
                    {
                        AnimStopPlaying();
                    }
                }
                
                nFrame = ClipRange(nFrame, 0, pSeq->nFrames - 1);
                pFrame = &pSeq->frames[nFrame];
                
                if (playing)
                {
                    if (pFrame->playSound)
                        SoundPlay(pSeq->nSoundID + Random(pFrame->soundRange));

                    if (pFrame->surfaceSound && viewSurface)
                        SoundPlay(kSurfSoundBase + (viewSurface<<1) + Random(2));
                }
            }
        }
        else
        {
            nFrame = ClipRange(nFrame, 0, pSeq->nFrames - 1);
            pFrame = &pSeq->frames[nFrame];
        }

        if (edit3d)
        {
            processMove();
            processMouseLook3D();
            processDrawRooms();
        }
        else
        {
            gfxSetColor(clr2std(kColorGrey29));
            gfxFillBox(windowx1, windowy1, windowx2, windowy2);
            FrameDraw(pFrame);
        }

        ObjectUpdate();
        HudShowInfo();
        handleevents();
        keyGetHelper(&key, &ctrl, &shift, &alt);
        OSD_Draw();
        showframe();
        
        if (key)
            keyClear();

        // GLOBAL keys
        //////////////////

        switch(key)
        {
            case KEY_ESC:
                if (playing)
                {
                    AnimStopPlaying();
                    continue;
                }

                if (!gArtEd.asksave && CRC == crc32once((unsigned char*)pSeq, kAnimSize))
                {
                    if (Confirm("Quit now?"))
                        return;

                    continue;
                }

                if ((i = DlgSaveChanges("Save changes?", gArtEd.asksave)) == -1) continue;
                else if (i == 0)
                    return;
                
                // no break
            case KEY_F2:
                if ((ctrl || !fileExists(gPaths.seqs)) && !browseSave(gPaths.seqs, kSeqExt)) continue;
                else if (!AnimSave(gPaths.seqs)) Alert("Failed to save file \"%s\"", gPaths.seqs);
                else
                {
                    rffID = -1;
                    pHud->SetMsgImp(MSG_TIME, "Saved to \"%s\"", gPaths.seqs);
                    
                    if (key == KEY_ESC)
                    {
                        if (i > 1)
                            artedSaveChanges();
                        
                        // quitting now
                        return;
                    }
                    
                    if (gArtEd.asksave && Confirm("Save ART changes as well?"))
                        artedSaveChanges();
                }
                continue;
            case KEY_F11: // load previous existing fileID in rff
            case KEY_F12: // load next existing fileID in rff
            case KEY_F3:  // call load dialog
                if (key == KEY_F3)
                {
                    if (ctrl)
                    {
                        if (Confirm("Start new animation?"))
                            AnimNew();

                        continue;
                    }
                    
                    if (!browseOpen(gPaths.seqs, kSeqExt))
                        continue;
                }
                else
                {
                    if (ctrl)
                    {
                        gHudPrefs.fontPack = IncRotate(gHudPrefs.fontPack, kHudFontPackMax);
                        pHud->SetMsgImp(MSG_TIME, "HUD font: %s", gHudFontNames[gHudPrefs.fontPack]);
                        hudSetFontPack(pHud, gHudPrefs.fontPack, &gMouse);
                        continue;
                    }
                    
                    i = getClosestId(ClipRange(rffID, 0, 65534), 65534, gExtNames[kSeq], key == KEY_F12);
                    sprintf(gPaths.seqs, "%d.%s", i, gExtNames[kSeq]);
                }
                
                if (AnimLoad(gPaths.seqs))
                {
                    pHud->SetMsgImp(MSG_TIME, "File \"%s\" loaded from %s", gPaths.seqs, (rffID >= 0) ? gPaths.bloodRFF : "disk");
                    HudUpdateStatus();
                    AnimStopPlaying();
                    ObjectReset();
                    continue;
                }
                
                Alert("Failed to load file \"%s\"!", gPaths.seqs);
                continue;
            case KEY_F4:// import
                j = rffID;
                strcpy(buffer, gPaths.seqs);
                if (browseOpen(gPaths.seqs, kSeqExt, "Import animation") != NULL)
                {
                    Seq* pTmp = NULL;
                    if (AnimLoad(gPaths.seqs, &pTmp))
                    {
                        for (i = 0, k = pSeq->nFrames; i < pTmp->nFrames; i++, k++)
                        {
                            if (k >= kMaxFrames)
                            {
                                Alert("Max (%d) frames reached!", kMaxFrames);
                                break;
                            }

                            pSeq->frames[k] = pTmp->frames[i];
                            pSeq->nFrames++;
                        }

                        Alert("%d of %d frames has been added.", i, pTmp->nFrames);
                        if (i > 0) nFrame = pSeq->nFrames - i;
                        HudUpdateStatus();
                    }
                    else
                    {
                        Alert("Failed to load file \"%s\"!", gPaths.seqs);
                    }

                    if (pTmp)
                        free(pTmp);
                }
                strcpy(gPaths.seqs, buffer);
                rffID = j;
                continue;
            case KEY_SPACE:
                if (pSeq->nFrames)
                {
                    if (playing)
                    {
                        AnimStopPlaying();
                        continue;
                    }

                    AnimStartPlaying((shift) ? 0x00 : 0x02);
                }
                continue;
            case KEY_PADENTER:
                if (obj.type != OBJ_NONE)
                {
                    edit3d = !edit3d;
                    pHud->SetMsgImp(MSG_TIME, "3D Mode is %s", onOff(edit3d));
                    HudUpdateStatus();
                    continue;
                }

                pHud->SetMsgImp(MSG_TIME, "3D Mode is not available");
                edit3d = 0;
                continue;
            case KEY_CAPSLOCK:
                if (!edit3d)
                    viewBorders = !viewBorders;
                continue;
            case KEY_SLASH:
            case KEY_COMMA:
            case KEY_PERIOD:
                if (pSeq->nFrames)
                {
                    if (panm[seqGetTile(pFrame)].view == kSprViewSingle)
                    {
                        pHud->SetMsgImp(MSG_TIME, "Tile #%d has no view type.", seqGetTile(pFrame));
                        continue;
                    }

                    if (key == KEY_COMMA)
                    {
                        if (pFrame->xflip)  viewOctant = IncRotate(viewOctant, 8);
                        else                viewOctant = DecRotate(viewOctant, 8);
                    }
                    else if (key == KEY_PERIOD)
                    {
                        if (pFrame->xflip)  viewOctant = DecRotate(viewOctant, 8);
                        else                viewOctant = IncRotate(viewOctant, 8);
                    }
                    else
                    {
                        viewOctant = 0;
                    }

                    if (obj.type == OBJ_SPRITE)
                    {
                        sprite[obj.index].ang  = (ang + kAng180) & kAngMask;
                        sprite[obj.index].ang -= (kAng45 * viewOctant);
                    }
                }
                continue;
            case KEY_PADSTAR:
            case KEY_PADSLASH:
                if (pSeq->nFrames)
                {
                    if (edit3d)
                    {
                       if (obj.type == OBJ_SPRITE)
                       {
                            spritetype* pSpr = &sprite[obj.index];
                            XSPRITE* pXSpr = &xsprite[pSpr->extra];
                            pSpr->xrepeat = refinfo.xsiz;
                            pSpr->yrepeat = refinfo.ysiz;

                            if (!pXSpr->scale)
                                pXSpr->scale = 256;

                            nStep = (key == KEY_PADSTAR) ? 8 : -8;
                            pXSpr->scale  = ClipRange(pXSpr->scale + nStep, 1, 256<<4);
                            pSpr->xrepeat = ClipRange(mulscale8(pSpr->xrepeat, pXSpr->scale), 0, 255);
                            pSpr->yrepeat = ClipRange(mulscale8(pSpr->yrepeat, pXSpr->scale), 0, 255);
                            ObjectUpdate();
                       }
                    }
                    else
                    {
                        nStep = (key == KEY_PADSTAR) ? 0x1000 : -0x1000;
                        zoom = ClipRange(zoom + nStep, 0, 0x10000 << 2);
                    }

                    HudUpdateStatus();
                }
                continue;
            case KEY_PLUS:
                if (ctrl)
                {
                    viewSurface = IncRotate(viewSurface, LENGTH(surfNames));
                    HudUpdateStatus();
                }
                else
                {
                    pSeq->ticksPerFrame = ClipHigh(pSeq->ticksPerFrame + 1, 32767);
                }
                continue;
            case KEY_MINUS:
                if (ctrl)
                {
                    viewSurface = DecRotate(viewSurface, LENGTH(surfNames));
                    HudUpdateStatus();
                }
                else
                {
                    pSeq->ticksPerFrame = ClipLow(pSeq->ticksPerFrame - 1, 1);
                }
                continue;
            case KEY_PADMINUS:
            case KEY_PADPLUS:
            case KEY_PAD0:
                if (shift)
                {
                    if (key == KEY_PADMINUS)        viewShade = (ctrl) ? +127 : ClipHigh(viewShade + 1, 127);
                    else if (key == KEY_PADPLUS)    viewShade = (ctrl) ? -128 : ClipLow(viewShade - 1, -128);
                    else                            viewShade = 0;

                    HudUpdateStatus();

                    if (pViewXSect)
                        pViewXSect->amplitude = viewShade;
                }
                else if (!playing && pSeq->nFrames)
                {
                    if (alt && key != KEY_PAD0)
                    {
                        i = GetNumberBox("Frame shade", pFrame->shade, pFrame->shade);
                        pFrame->shade = ClipRange(i, -128, 127);
                        continue;
                    }

                    if (key == KEY_PADMINUS)        pFrame->shade = (ctrl) ? +127 : ClipHigh(pFrame->shade + 1, 127);
                    else if (key == KEY_PADPLUS)    pFrame->shade = (ctrl) ? -128 : ClipLow(pFrame->shade - 1, -128);
                    else                            pFrame->shade = 0;
                }
                continue;
            case KEY_BACKSLASH:
                if (edit3d && obj.type == OBJ_SPRITE)
                {
                    viewZ = IncRotate(viewZ, 4);
                    HudUpdateStatus();
                }
                break;
        }

        if (playing)
            continue;

        //// GLOBAL editing
        ////////////////////////////

        switch(key)
        {
            case KEY_L:
                pSeq->flags ^= kSeqLoop;
                pSeq->flags &= ~kSeqRemove;
                HudUpdateStatus();
                continue;
            case KEY_D:
                pSeq->flags ^= kSeqRemove;
                pSeq->flags &= ~kSeqLoop;
                HudUpdateStatus();
                continue;
            case KEY_V:
                if (pSeq->nFrames)
                {
                    if ((nTile = tilePick(seqGetTile(pFrame), -1, OBJ_ALL, "Frame picnum")) >= 0)
                        seqSetTile(pFrame, nTile);

                    continue;
                }
                // no break
            case KEY_INSERT:
                if (pSeq->nFrames < kMaxFrames)
                {
                    if ((nTile = tilePick((pSeq->nFrames) ? seqGetTile(pFrame) : 0, -1, OBJ_ALL, "New frame")) < 0)
                        continue;

                    if (pSeq->nFrames)
                    {
                        if (!ctrl)
                            nFrame++; // insert next frame AFTER current (BEFORE otherwise)

                        for (i = pSeq->nFrames; i >= nFrame && i-1 >= 0; i--)
                            pSeq->frames[i] = pSeq->frames[i-1];
                    }
                    else
                    {
                        pFrame = pSeq->frames;
                        FrameClean(pFrame);
                        nFrame = 0;
                    }

                    pSeq->frames[nFrame].playSound    = 0;
                    pSeq->frames[nFrame].surfaceSound = 0;
                    pSeq->frames[nFrame].soundRange   = 0;
                    seqSetTile(&pSeq->frames[nFrame], nTile);
                    pSeq->nFrames++;

                    pHud->SetMsgImp(MSG_TIME, "Frame created");
                }
                continue;
            case KEY_S:
                if (alt || !pSeq->nSoundID)
                    pSeq->nSoundID = ClipRange(GetNumberBox("Global sound ID", pSeq->nSoundID, pSeq->nSoundID), 0, 32767);

                if (pSeq->nFrames && pSeq->nSoundID)
                {
                    if (ctrl)
                    {
                        pFrame->soundRange = ClipRange(GetNumberBox("Sound ID range", pFrame->soundRange, pFrame->soundRange), 0, 15);
                        continue;
                    }

                    if (!alt)
                        pFrame->playSound = !pFrame->playSound;
                }
                continue;
            case KEY_F5:
                if (alt && pSeq->nFrames)
                {
                    artedDlgViewTypeSet(seqGetTile(pFrame));
                    continue;
                }
                HudEditInfo();
                continue;
            case KEY_F6:
                if (alt && pSeq->nFrames)
                {
                    nTile = seqGetTile(pFrame);
                    if ((i = showButtons(surfNames, LENGTH(surfNames), "Tile surface type")) >= mrUser)
                    {
                        surfType[nTile] = i - mrUser;
                        artedArtDirty(nTile, kDirtyDat);
                    }

                    continue;
                }
                HudEditInfo();
                continue;
            case KEY_F10:
                auditSound
                (
                pSeq->nSoundID + Random((ctrl && pSeq->nFrames) ? pFrame->soundRange : 0),
                kSoundPlayer,
                buffer
                );
                pHud->SetMsgImp(MSG_TIME, buffer);
                continue;
            case KEY_ENTER:
                if (!clipboard.ok)
                {
                    pHud->SetMsgImp(MSG_TIME, "There is nothing to paste.");
                    continue;
                }

                if (!shift)
                {
                    if (pSeq->nFrames <= 0)
                        pFrame = pSeq->frames, nFrame = 0, pSeq->nFrames++;

                    memcpy(pFrame, &clipboard.frame, sizeof(SEQFRAME));
                    pHud->SetMsgImp(MSG_TIME, "Paste to frame #%d", nFrame+1);
                    ObjectReset();
                    continue;
                }

                while ( 1 )
                {
                    if ((i = createCheckboxList(clipboardMenu, LENGTH(clipboardMenu), "Paste properties", TRUE)) == 0)
                        break;

                    if (pSeq->nFrames > 1)
                    {
                        if ((j = showButtons(pasteDirection, LENGTH(pasteDirection), "Select direction")) < mrUser)
                            continue;

                        switch (j -= mrUser)
                        {
                            case 0: i = nFrame;     break;
                            case 1: i = nFrame + 1; break;
                            case 2: i = nFrame - 1; break;
                            case 3: i = 0;          break;
                        }
                    }
                    else
                    {
                        if (pSeq->nFrames <= 0)
                            pFrame = pSeq->frames, nFrame = 0, pSeq->nFrames++;

                        i = nFrame;
                        j = 0;
                    }

                    i = ClipRange(i, 0, pSeq->nFrames - 1);
                    SEQFRAME* pBuffer = &clipboard.frame;

                    while ( 1 )
                    {
                        SEQFRAME* pFrame = &pSeq->frames[i];

                        if (clipboardMenu[0].option)
                        {
                            pFrame->tile  = pBuffer->tile;
                            pFrame->tile2 = pBuffer->tile2;
                        }

                        if (clipboardMenu[1].option) pFrame->xrepeat    = pBuffer->xrepeat;
                        if (clipboardMenu[2].option) pFrame->yrepeat    = pBuffer->yrepeat;
                        if (clipboardMenu[3].option) pFrame->shade      = pBuffer->shade;
                        if (clipboardMenu[4].option) pFrame->pal        = pBuffer->pal;
                        if (clipboardMenu[5].option)
                        {
                            pFrame->xflip           = pBuffer->xflip;
                            pFrame->yflip           = pBuffer->yflip;
                            pFrame->autoaim         = pBuffer->autoaim;
                            pFrame->blockable       = pBuffer->blockable;
                            pFrame->invisible       = pBuffer->invisible;
                            pFrame->hittable        = pBuffer->hittable;
                            pFrame->smoke           = pBuffer->smoke;
                            pFrame->transparent     = pBuffer->transparent;
                            pFrame->transparent2    = pBuffer->transparent2;
                            pFrame->pushable        = pBuffer->pushable;

                            if (j == 0)
                            {
                                pFrame->surfaceSound    = pBuffer->surfaceSound;
                                pFrame->trigger         = pBuffer->trigger;
                                pFrame->playSound       = pBuffer->playSound;
                            }
                        }

                        if ((j == 2 && --i >= 0) ||
                            ((j == 1 || j == 3) && ++i < pSeq->nFrames))
                                continue;

                        break;
                    }

                    pHud->SetMsgImp(MSG_TIME, "Paste selected properties.");
                    ObjectReset();
                    break;
                }
                continue;
        }

        if (pSeq->nFrames <= 0)
            continue;

        //// FRAME editing
        ////////////////////////////

        switch(key)
        {
            case KEY_HOME:  nFrame = 0;                                                                 continue;
            case KEY_END:   nFrame = ClipLow(pSeq->nFrames - 1, 0);                                     continue;
            case KEY_X:     pFrame->xflip           = !pFrame->xflip;                                   continue;
            case KEY_Y:     pFrame->yflip           = !pFrame->yflip;                                   continue;
            case KEY_M:     pFrame->autoaim         = !pFrame->autoaim;                                 continue;
            case KEY_B:     pFrame->blockable       = !pFrame->blockable;                               continue;
            case KEY_I:     pFrame->invisible       = !pFrame->invisible;                               continue;
            case KEY_H:     pFrame->hittable        = !pFrame->hittable;                                continue;
            case KEY_K:     pFrame->smoke           = !pFrame->smoke;                                   continue;
            case KEY_U:     pFrame->surfaceSound    = !pFrame->surfaceSound;                            continue;
            case KEY_T:     pFrame->trigger         = !pFrame->trigger;                                 continue;
            case KEY_R:
                if (!pFrame->transparent)
                {
                    pFrame->transparent  = 1;
                }
                else if (!pFrame->transparent2)
                {
                    pFrame->transparent2 = 1;
                }
                else
                {
                    pFrame->transparent  = 0;
                    pFrame->transparent2 = 0;
                }
                continue;
            case KEY_P:
                if (alt || shift)
                {
                    if (alt) pFrame->pal = pluPickAdvanced(seqGetTile(pFrame), pFrame->shade, pFrame->pal, "Palookup");
                    else if (shift & 0x01) pFrame->pal = getClosestId(pFrame->pal, kPluMax - 1, "PLU", TRUE);
                    else if (shift & 0x02) pFrame->pal = getClosestId(pFrame->pal, kPluMax - 1, "PLU", FALSE);

                    if (pFrame->pal == 0)
                        viewPal = 0, ObjectReset();

                    continue;
                }
                pFrame->pushable = !pFrame->pushable;
                continue;
            case KEY_1:
                nFrame = ClipLow(nFrame - 1, 0);
                continue;
            case KEY_2:
                nFrame = ClipHigh(nFrame + 1, pSeq->nFrames-1);
                continue;
            case KEY_G:
                i = GetNumberBox("Goto frame", nFrame+1, nFrame+1);
                nFrame = ClipRange(i - 1, 0, pSeq->nFrames-1);
                continue;
            case KEY_DELETE:
                pSeq->nFrames--;
                for (i = nFrame; i < pSeq->nFrames; i++)
                    pSeq->frames[i] = pSeq->frames[i+1];

                nFrame = ClipHigh(nFrame, pSeq->nFrames-1);
                continue;
            case KEY_F9: // reverse frames order
                if (pSeq->nFrames > 1)
                {
                    for (i = 0; i < pSeq->nFrames >> 1; i++)
                    {
                        SEQFRAME cpyframe = pSeq->frames[i];
                        pSeq->frames[i] = pSeq->frames[pSeq->nFrames-i-1];
                        pSeq->frames[pSeq->nFrames-i-1] = cpyframe;
                    }

                    pHud->SetMsgImp(MSG_TIME, "Reverse frames order.");
                }
                continue;
            case KEY_PADUP:
            case KEY_PADDOWN:
            case KEY_PADLEFT:
            case KEY_PADRIGHT:
            case KEY_PAD7:
            case KEY_PAD9:
                if (alt && key != KEY_PAD7 && key != KEY_PAD9)
                {
                    if (key == KEY_PADDOWN || key == KEY_PADUP)
                    {
                        pFrame->yrepeat = ClipRange(GetNumberBox("Frame Y-Repeat", pFrame->yrepeat, pFrame->yrepeat), 0, 255);
                    }
                    else
                    {
                        pFrame->xrepeat = ClipRange(GetNumberBox("Frame X-Repeat", pFrame->xrepeat, pFrame->xrepeat), 0, 255);
                    }

                    continue;
                }
                else
                {
                    nStep = (shift) ? 1 : 4;
                    if (key == KEY_PADRIGHT || key == KEY_PAD9)  pFrame->xrepeat = (ctrl) ? 255 : IncNext(pFrame->xrepeat, nStep);
                    if (key == KEY_PADUP    || key == KEY_PAD9)  pFrame->yrepeat = (ctrl) ? 255 : IncNext(pFrame->yrepeat, nStep);
                    if (key == KEY_PADLEFT  || key == KEY_PAD7)  pFrame->xrepeat = (ctrl) ? 0   : DecNext(pFrame->xrepeat, nStep);
                    if (key == KEY_PADDOWN  || key == KEY_PAD7)  pFrame->yrepeat = (ctrl) ? 0   : DecNext(pFrame->yrepeat, nStep);
                }
                ObjectReset();
                continue;
            case KEY_PAGEUP:
            case KEY_PAGEDN:
                nTile = seqGetTile(pFrame);
                do
                {
                   nTile = (key == KEY_PAGEDN)
                        ? IncRotate(nTile, gMaxTiles) : DecRotate(nTile, gMaxTiles);
                }
                while (!tileLoadTile(nTile));
                seqSetTile(pFrame, nTile);
                continue;
            case KEY_TAB:
                memcpy(&clipboard.frame, pFrame, sizeof(*pFrame));
                pHud->SetMsgImp(MSG_TIME, "Copy frame #%d.", nFrame+1);
                clipboard.ok = 1;
                continue;
        }

        if (!edit3d && totalclock > nKeyTime)
        {
            int nAng = 0;
            nTile = toolGetViewTile(seqGetTile(pFrame), viewOctant, NULL, &nAng);
            nKeyTime = totalclock + 6;

            int hg = tilesizy[nTile], hhg = hg >> 1, yo = panm[nTile].ycenter;
            int wh = tilesizx[nTile], hwh = wh >> 1, xo = panm[nTile].xcenter;
            char xf = (pFrame->xflip || nAng == kAng180);
            char yf = (pFrame->yflip);

            i = 0;
            if (!keystatus[KEY_PAD5])
            {
                char rt = keystatus[KEY_RIGHT], lt = keystatus[KEY_LEFT];
                char up = keystatus[KEY_UP],    dn = keystatus[KEY_DOWN];
                
                if ((up && !yf) || (dn && yf))  yo = ClipHigh((shift) ? ((yo >= 0) ? hhg : 0) : yo + 1, +127), i = 1;
                if ((dn && !yf) || (up && yf))  yo = ClipLow((shift) ? ((yo <= 0) ? -hhg : 0) : yo - 1, -128), i = 1;
                if ((lt && !xf) || (rt && xf))  xo = ClipHigh((shift) ? ((xo >= 0) ? hwh : 0) : xo + 1, +127), i = 1;
                if ((rt && !xf) || (lt && xf))  xo = ClipLow((shift) ? ((xo <= 0) ? -hwh : 0) : xo - 1, -128), i = 1;
            }
            else
            {
                xo = 0;
                yo = 0;
                i  = 1;
            }

            if (i)
            {
                panm[nTile].ycenter = yo, panm[nTile].xcenter = xo;
                artedArtDirty(nTile, kDirtyPicanm);
                continue;
            }
        }
    }

    return;
}

void SEQEDIT::Quit()
{
    if (pSeq)
        free(pSeq), pSeq = NULL;

    clipboard.ok = 0;
}

char SEQEDIT::AnimNew()
{
    int i;

    if (pSeq)
        free(pSeq);

    pSeq = (Seq*)malloc(kAnimSize);
    dassert(pSeq != NULL);

    memset(pSeq, 0, kAnimSize);

    pSeq->nFrames       = 1;
    pSeq->ticksPerFrame = 12;
    rffID               = -1;
    CRC                 = 0;

    i = kMaxFrames;
    while(--i >= 0)
        FrameClean(&pSeq->frames[i]);

    return 1;
}

char SEQEDIT::AnimLoad(char* filename, Seq** pOut)
{
    RESHANDLE hRes = NULL; Seq *pTmp, *pRFFAnim;
    char edit = (pOut == NULL), r = 0;
    int hFile, i, nSize;

    if (pOut != NULL)
        dassert(*pOut == NULL);

    if ((i = fileExists(filename, &hRes)) <= 0
        || (pTmp = (Seq*)malloc(kAnimSize)) == NULL)
            return 0;

    memset(pTmp, 0, kAnimSize);

    // first try load from the disk
    if ((i & 0x01) && ((hFile = open(filename, O_RDONLY|O_BINARY, S_IWRITE|S_IREAD)) >= 0))
    {
        nSize = ClipHigh(filelength(hFile), kAnimSize);
        read(hFile, pTmp, nSize);
        close(hFile);
        if (edit)
            rffID = -1;
    }
    else if (hRes != NULL) // rff otherwise
    {
        pRFFAnim = (Seq*)gSysRes.Load(hRes);
        nSize = ClipHigh(gSysRes.Size(hRes), kAnimSize);
        memcpy(pTmp, pRFFAnim, nSize);
        if (edit)
        {
            strcpy(gPaths.seqs, hRes->name);
            rffID = hRes->id;
        }
    }

    if ((r = (memcmp(pTmp->signature, kSEQSig, LENGTH(pTmp->signature)) == 0)) != 0)
    {
        if (edit)
        {
            if (!pSeq)
                AnimNew();

            memcpy(pSeq, pTmp, nSize), free(pTmp);
            CRC = crc32once((unsigned char*)pSeq, kAnimSize); // save to compare changes
        }
        else
        {
            *pOut = pTmp;
        }
    }
    else
    {
       free(pTmp);
    }

    return r;
}

char SEQEDIT::AnimSave(char* filename)
{
    SEQFRAME* p;
    int hFile, i;

    if (filename == NULL)
        return 0;

    ChangeExtension(filename, kSeqExt);
    if ((hFile = open(filename, O_CREAT|O_WRONLY|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE)) == -1)
        return 0;

    pSeq->version = kSeqVersion;

    for (i = 0; i < pSeq->nFrames; i++)
    {
        p = &pSeq->frames[i];
        if (seqGetTile(p) >= 4095 || p->surfaceSound || p->soundRange)
        {
            pSeq->version = kSeqVersionExt1;
            break;
        }
    }

    for (i = 0; i < pSeq->nFrames; i++)
    {
        p = &pSeq->frames[i];
        if (seqGetPal(p) >= 32)
        {
            pSeq->version = kSeqVersionExt2;
            break;
        }
    }

    memcpy(pSeq->signature, kSEQSig, sizeof(pSeq->signature));
    write(hFile, pSeq, sizeof(Seq) + pSeq->nFrames * sizeof(SEQFRAME));
    close(hFile);
    
    CRC = crc32once((unsigned char*)pSeq, kAnimSize);
    return 1;
}

void SEQEDIT::AnimStartPlaying(char flags)
{
    SoundStop();

    playing = (flags) ? flags
            : (pSeq->flags & kSeqLoop) ? 0x02 : 0x01;

    nFrame = 0;
}

void SEQEDIT::AnimStopPlaying()
{
    SoundStop();
    playing = 0;
    nFrame = 0;
}

void SEQEDIT::FrameClean(SEQFRAME* pFrame)
{
    memset(pFrame, 0, sizeof(*pFrame));

    pFrame->hittable    = 1;
    pFrame->blockable   = 1;
    pFrame->autoaim     = 1;
    pFrame->shade       = 8;
}

void SEQEDIT::FrameDraw(SEQFRAME* pFrame)
{
    ROMFONT* pFont = pHud->GetFont(kHudFontMsg);
    int nShade, nTile, nVTile, nVPal, nZoom, nAng;
    int wh, hg, sz, i;
    int x[4], y[4];

    char c = clr2std(kColorLightCyan);
    static char buf[16];
    char flags = 0;

    hg = ydim-pHud->Height();
    nZoom = mulscale8(0x10000, perc2val(75, hg)) + zoom;
    sz = perc2val(40, hg);

    if (pSeq->nFrames <= 0)
    {
        toolDrawCenter(&origin, c, sz, sz, 4);
        return;
    }

    if (pFrame->pal != 0)
        viewPal = pFrame->pal;

    nVPal  = (pFrame->pal == 0) ? viewPal : pFrame->pal;
    nShade = ClipRange(viewShade + pFrame->shade, -128, 127);
    nTile  = seqGetTile(pFrame);
    nAng   = 0;

    if (pFrame->transparent)                flags |= kRSTransluc;
    if (pFrame->transparent2)               flags |= kRSTranslucR;

    if (pFrame->xflip && pFrame->yflip)     nAng += kAng180;
    else if (pFrame->xflip)                 nAng += kAng180, flags |= kRSYFlip;
    else if (pFrame->yflip)                 flags |= kRSYFlip;

    nVTile = toolGetViewTile(nTile, viewOctant, &flags, &nAng);
    toolDrawCenter(&origin, c, sz, sz, 4);

    if (!pFrame->invisible)
    {
        rotatesprite(origin.x << 16, origin.y << 16, nZoom, nAng, nVTile, nShade, nVPal, flags, 0, 0, xdim-1, ydim-1);

        if (viewBorders)
        {
            gfxSetColor(clr2std(kColorGrey26));
            GetRotateSpriteExtents(origin.x, origin.y, nZoom, nAng, nVTile, flags, xdim-1, ydim-1, x, y);
            gfxLine(x[0], y[0], x[1], y[1]), gfxLine(x[1], y[1], x[2], y[2]);
            gfxLine(x[2], y[2], x[3], y[3]), gfxLine(x[3], y[3], x[0], y[0]);
        }
    }

    wh = pFont->wh, hg = pFont->hg;
    sprintf(buf, "X:%+03d", panm[nVTile].xcenter);
    printextShadow(origin.x + sz + wh, origin.y - (hg >> 1), c, buf, pFont);

    i = (sprintf(buf, "Y:%+03d", panm[nVTile].ycenter) * pFont->ls) >> 1;
    printextShadow(origin.x - i, origin.y + sz + hg, c, buf, pFont);

    switch (viewType[nTile])
    {
        case kSprViewFull5:
        case kSprViewFull8:
            i = (sprintf(buf, "ANG:%03ddeg", viewOctant * 45) * pFont->ls)>>1;
            printextShadow(origin.x - i, origin.y + sz + (hg << 1), c, buf, pFont);
            break;
    }

    pHud->SetTile(nVTile, nVPal, nShade);
}

void SEQEDIT::HudShowInfo()
{
    int nVTile, nVPal, nShade, i;
    static char* onDone[] = {"Stop", "Loop", "Delete"};
    static char buffer[256], filename[BMAX_PATH];
    char* p = buffer;

    getFilename(gPaths.seqs, filename, 0);
    strcat(filename, kSeqExt);
    strupr(filename);

    if (playing)
        p += sprintf(p, "[%s] ", "PLAY");

    p += sprintf(p, "\"%s\"", filename);
    if (rffID >= 0)
        p += sprintf(p, " (ID #%d)", rffID);
    else
        p += sprintf(p, " (%s)", "DISK");

    p += sprintf(p, "%s ", ":");

    if (pSeq->nFrames > 0)
    {
        p += sprintf(p, "Frame=%d/%d", nFrame+1, pSeq->nFrames);

        i = (ClipLow(pSeq->nFrames, 0))*pSeq->ticksPerFrame;
        p += sprintf(p, ", Length=%02d:%03d%s", i/120, i%120, (i/120 == 0) ? "ms" : "s");

        if (rngok(pSeq->flags & 0x03, 0x00, 0x03))
            p += sprintf(p, ", When done=%s", onDone[pSeq->flags & 0x03]);
    }
    else
    {
        p += sprintf(p, "%s", "No frames!");
    }


    if (!pSeq->nFrames || playing)
        pHud->SetMsgImp(16, buffer);
    else
        pHud->SetMsg(buffer);

    if (pSeq->nFrames)
    {
        nVTile = toolGetViewTile(seqGetTile(pFrame), viewOctant, NULL, NULL);
        nVPal  = (pFrame->pal == 0) ? viewPal : pFrame->pal;
        nShade = ClipRange(viewShade + pFrame->shade, -128, 127);

        pHud->SetTile(nVTile, nVPal, nShade);
    }
    else
    {
        pHud->SetTile();
    }

    pHud->DrawIt();

    DIALOG_HANDLER dialog(pHud, dlgSeqedit);
    dlgSeqToDialog(&dialog, pSeq, pFrame);
    dialog.Paint();
}

void SEQEDIT::HudEditInfo()
{
    int t = pSeq->nFrames;
    int i;
    
    pHud->DrawIt();
    DIALOG_HANDLER dialog(pHud, dlgSeqedit);
    dlgSeqToDialog(&dialog, pSeq, pFrame);

    if (dialog.Edit())
    {
        dlgDialogToSeq(&dialog, pSeq, pFrame);
        nFrame = ClipRange(nFrame, 0, pSeq->nFrames - 1);
        pFrame = &pSeq->frames[nFrame];
        
        i = pSeq->nFrames;
        while(--i >= t)
            FrameClean(&pSeq->frames[t]);
    }
}

void SEQEDIT::HudUpdateStatus()
{
    int nZoom;

    statViewSurface.id  = 1;
    statViewShade.id    = 2;
    statViewZoom.id     = 3;
    statViewZ.id        = 4;

    pHud->StatusRem(&statViewShade);
    pHud->StatusRem(&statViewSurface);
    pHud->StatusRem(&statViewZoom);
    pHud->StatusRem(&statViewZ);

    if (viewSurface && obj.type == OBJ_SPRITE)
    {
        sprintf(statViewSurface.text, "Surface: %s", surfNames[viewSurface]);
        statViewSurface.color[0][0] = clr2std(kColorGreen);
        statViewSurface.color[1][0] = clr2std(kColorGrey18);
        pHud->StatusAdd(&statViewSurface);
    }

    if (viewShade)
    {
        sprintf(statViewShade.text, "Shade: %+d", viewShade);
        statViewShade.color[0][0] = clr2std(kColorGrey28);
        statViewShade.color[1][0] = clr2std(kColorGrey18);
        pHud->StatusAdd(&statViewShade);
    }

    if (edit3d)
    {
        if (obj.type == OBJ_SPRITE)
        {
            sprintf(statViewZ.text, "Z: %s", viewZNames[viewZ]);
            statViewZ.color[0][0] = clr2std(kColorBlue);
            statViewZ.color[1][0] = clr2std(kColorGrey18);
            pHud->StatusAdd(&statViewZ);

            XSPRITE* pXSpr = &xsprite[sprite[obj.index].extra];
            if (pXSpr->scale && (nZoom = IVAL2PERC(pXSpr->scale, 256)) != 100)
            {
                sprintf(statViewZoom.text, "Scale: %d%%", nZoom);
                statViewZoom.color[0][0] = clr2std(kColorRed);
                statViewZoom.color[1][0] = clr2std(kColorGrey18);
                pHud->StatusAdd(&statViewZoom);
            }
        }
    }
    else if ((nZoom = IVAL2PERC(zoom, GetDefaultZoom())) > 0)
    {
        sprintf(statViewZoom.text, "Zoom: %d%%", nZoom); strupr(statViewZoom.text);
        statViewZoom.color[0][0] = clr2std(kColorRed);
        statViewZoom.color[1][0] = clr2std(kColorGrey18);
        pHud->StatusAdd(&statViewZoom);
    }
}

void SEQEDIT::ObjectUpdate(char nType)
{
    spritetype* pSpr; sectortype* pSect; walltype* pWall;
    int cz, fz, zt, zb;
    int dx, dy;
    
    switch (obj.type)
    {
        case OBJ_SPRITE:
            pSpr = &sprite[obj.index];

            if (pSeq->nFrames)
            {
                if (nType == 1)
                {
                    if (pFrame->xrepeat == 0)   pSpr->xrepeat = refinfo.xsiz;
                    if (pFrame->yrepeat == 0)   pSpr->yrepeat = refinfo.ysiz;
                    if (pFrame->pal == 0)       pSpr->pal = 0;
                }

                UpdateSprite(pSpr->extra, pFrame);
            }
            else
            {
                pSpr->xrepeat = refinfo.xsiz;
                pSpr->yrepeat = refinfo.ysiz;
                pSpr->picnum  = refinfo.pic;
                pSpr->pal     = refinfo.pal;
                pSpr->flags   = 0;
            }

            getzsofslope(pSpr->sectnum, pSpr->x, pSpr->y, &cz, &fz);
            GetSpriteExtents(pSpr, &zt, &zb);

            switch(viewZ)
            {
                case 0:  pSpr->z = fz - (zb - pSpr->z); break;
                case 1:  pSpr->z = MIDPOINT(fz, cz);    break;
                case 2:  pSpr->z = cz + (pSpr->z - zt); break;
                case 3:  pSpr->z = refinfo.z;           break;
            }
            
            dx = posx - pSpr->x, dy = posy - pSpr->y;
            RotateVector(&dx, &dy, -pSpr->ang + kAng45 / 2);
            viewOctant = GetOctant(dx, dy);
            
            break;
        case OBJ_FLOOR:
            pSect = &sector[obj.index];

            if (pSeq->nFrames)
            {
                if (nType == 1)
                {
                    if (pFrame->pal == 0)   pSect->floorpal = 0;
                }

                UpdateFloor(pSect->extra, pFrame);
            }
            else
            {
                pSect->floorpicnum  = refinfo.pic;
                pSect->floorpal     = refinfo.pal;
            }

            break;
        case OBJ_CEILING:
            pSect = &sector[obj.index];

            if (pSeq->nFrames)
            {
                if (nType == 1)
                {
                    if (pFrame->pal == 0)   pSect->ceilingpal = 0;
                }

                UpdateCeiling(pSect->extra, pFrame);
            }
            else
            {
                pSect->ceilingpicnum  = refinfo.pic;
                pSect->ceilingpal     = refinfo.pal;
            }

            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            pWall = &wall[obj.index];

            if (pSeq->nFrames)
            {
                if (nType == 1)
                {
                    if (pFrame->pal == 0)   pWall->pal = 0;
                }

                (obj.type == OBJ_WALL)
                    ? UpdateWall(pWall->extra, pFrame)
                    : UpdateMasked(pWall->extra, pFrame);
            }
            else if (obj.type == OBJ_WALL)
            {
                pWall->picnum       = refinfo.pic;
                pWall->pal          = refinfo.pal;
            }
            else
            {
                pWall->overpicnum   = refinfo.pic;
                pWall->pal          = refinfo.pal;
            }

            break;
    }
}

void SEQEDIT::SoundPlay(int nID)
{
    switch(obj.type)
    {
        case OBJ_SPRITE:
            //SoundStop();
            sfxPlay3DSound(&sprite[obj.index], nID, -1, 0);
            break;
        case OBJ_NONE:
            //SoundStop();
            auditSound(nID, -1, (char)0);
            break;
    }
}

void SEQEDIT::SoundStop()
{
    sndKillAllSounds();
    sfxKillAllSounds();
}

static void dlgSeqToDialog(DIALOG_HANDLER* pHandle, Seq* pSeq, SEQFRAME* pFrame)
{
    pHandle->SetValue(kSeqDlgSpeed,         pSeq->ticksPerFrame);
    pHandle->SetValue(kSeqDlgSound,         pSeq->nSoundID);
    pHandle->SetValue(kSeqDlgFrames,        pSeq->nFrames);

    pHandle->SetValue(kSeqDlgRemove,        (pSeq->flags & kSeqRemove) != 0);
    pHandle->SetValue(kSeqDlgLoop,          (pSeq->flags & kSeqLoop) != 0);

    if (pSeq->nFrames)
    {
        pHandle->SetValue(kSeqDlgFramePic,      seqGetTile(pFrame));
        pHandle->SetValue(kSeqDlgFramePal,      seqGetPal(pFrame));
        pHandle->SetValue(kSeqDlgFrameXSiz,     pFrame->xrepeat);
        pHandle->SetValue(kSeqDlgFrameYSiz,     pFrame->yrepeat);
        pHandle->SetValue(kSeqDlgFrameShade,    pFrame->shade);
        pHandle->SetValue(kSeqDlgSndRange,      pFrame->soundRange);
        pHandle->SetValue(kSeqDlgFrameBLK,      pFrame->blockable);
        pHandle->SetValue(kSeqDlgFrameHSC,      pFrame->hittable);
        pHandle->SetValue(kSeqDlgFrameAIM,      pFrame->autoaim);
        pHandle->SetValue(kSeqDlgFrameTRG,      pFrame->trigger);
        pHandle->SetValue(kSeqDlgFramePSH,      pFrame->pushable);
        pHandle->SetValue(kSeqDlgFrameSMK,      pFrame->smoke);
        pHandle->SetValue(kSeqDlgFrameFLX,      pFrame->xflip);
        pHandle->SetValue(kSeqDlgFrameFLY,      pFrame->yflip);
        pHandle->SetValue(kSeqDlgFrameINV,      pFrame->invisible);
        pHandle->SetValue(kSeqDlgFrameSND,      pFrame->playSound);
        pHandle->SetValue(kSeqDlgFrameSRF,      pFrame->surfaceSound);
        
        if (pFrame->transparent2)
        {
            pHandle->SetValue(kSeqDlgFrameTR1,      0);
            pHandle->SetValue(kSeqDlgFrameTR2,      1);
        }
        else if (pFrame->transparent)
        {
            pHandle->SetValue(kSeqDlgFrameTR1,      1);
            pHandle->SetValue(kSeqDlgFrameTR2,      0);
        }
        else
        {
            pHandle->SetValue(kSeqDlgFrameTR1,      0);
            pHandle->SetValue(kSeqDlgFrameTR2,      0);
        }
    }
}

static void dlgDialogToSeq(DIALOG_HANDLER* pHandle, Seq* pSeq, SEQFRAME* pFrame)
{
    pSeq->ticksPerFrame     = pHandle->GetValue(kSeqDlgSpeed);
    pSeq->nSoundID          = pHandle->GetValue(kSeqDlgSound);
    pSeq->nFrames           = pHandle->GetValue(kSeqDlgFrames);

    if (pHandle->GetValue(kSeqDlgRemove))
    {
        pSeq->flags |= kSeqRemove;
        pSeq->flags &= ~kSeqLoop;
    }
    else
        pSeq->flags &= ~kSeqRemove;

    if (pHandle->GetValue(kSeqDlgLoop))
    {
        pSeq->flags &= ~kSeqRemove;
        pSeq->flags |= kSeqLoop;
    }
    else
        pSeq->flags &= ~kSeqLoop;

    if (pSeq->nFrames)
    {
        seqSetTile(pFrame, pHandle->GetValue(kSeqDlgFramePic));
        seqSetPal(pFrame, pHandle->GetValue(kSeqDlgFramePal));

        pFrame->xrepeat         = pHandle->GetValue(kSeqDlgFrameXSiz);
        pFrame->yrepeat         = pHandle->GetValue(kSeqDlgFrameYSiz);
        pFrame->shade           = pHandle->GetValue(kSeqDlgFrameShade);
        pFrame->soundRange      = pHandle->GetValue(kSeqDlgSndRange);

        pFrame->blockable       = pHandle->GetValue(kSeqDlgFrameBLK);
        pFrame->hittable        = pHandle->GetValue(kSeqDlgFrameHSC);
        pFrame->autoaim         = pHandle->GetValue(kSeqDlgFrameAIM);
        pFrame->trigger         = pHandle->GetValue(kSeqDlgFrameTRG);
        pFrame->pushable        = pHandle->GetValue(kSeqDlgFramePSH);
        pFrame->smoke           = pHandle->GetValue(kSeqDlgFrameSMK);
        
        if (pHandle->GetValue(kSeqDlgFrameTR2))
        {
            pFrame->transparent  = 1;
            pFrame->transparent2 = 1;
        }
        else if (pHandle->GetValue(kSeqDlgFrameTR1))
        {
            pFrame->transparent  = 1;
            pFrame->transparent2 = 0;
        }
        else
        {
            pFrame->transparent  = 0;
            pFrame->transparent2 = 0;
        }
        
        pFrame->xflip           = pHandle->GetValue(kSeqDlgFrameFLX);
        pFrame->yflip           = pHandle->GetValue(kSeqDlgFrameFLY);
        pFrame->invisible       = pHandle->GetValue(kSeqDlgFrameINV);
        pFrame->playSound       = pHandle->GetValue(kSeqDlgFrameSND);
        pFrame->surfaceSound    = pHandle->GetValue(kSeqDlgFrameSRF);
    }
}

static char helperToggleSeqFlags(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key)
{
    if (key == KEY_SPACE)
    {
        for (DIALOG_ITEM* p = dialog; p->type != CONTROL_END; p++)
        {
            if ((control->tabGroup == kSeqDlgRemove && p->tabGroup == kSeqDlgLoop)
                || (control->tabGroup == kSeqDlgLoop && p->tabGroup == kSeqDlgRemove))
                {
                    p->value = 0;
                    break;
                }
            
            if ((control->tabGroup == kSeqDlgFrameTR1 && p->tabGroup == kSeqDlgFrameTR2)
                || (control->tabGroup == kSeqDlgFrameTR2 && p->tabGroup == kSeqDlgFrameTR1))
                {
                    p->value = 0;
                    break;
                }
        }
    }

    return key;
}

static char helperAuditSound(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key)
{
    int i;
    char msg[128];

    switch (key)
    {
        case KEY_UP:
            for (i = control->value+1; i < 65535; i++)
            {
                if (gSoundRes.Lookup(i, "SFX") != NULL)
                {
                    control->value = i;
                    break;
                }
            }
            return 0;
        case KEY_DOWN:
            if (control->value == 1)
            {
                control->value = 0;
                return 0;
            }
            for (i = control->value-1; i > 0; i--)
            {
                if (gSoundRes.Lookup(i, "SFX") != NULL)
                {
                    control->value = i;
                    break;
                }
            }
            return 0;
        case KEY_F10:
        case KEY_SPACE:
            auditSound(control->value, -1, msg);
            gSeqedHud.SetMsg(msg);
            return 0;
    }

    return key;
}