/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
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

#include "common_game.h"
#include "xmpmaped.h"
#include "tile.h"
#include "xmpsnd.h"
#include "xmptools.h"
#include "xmpseqed.h"
#include "xmpconf.h"
#include "xmpevox.h"

#define kMar 4
#define kSurfSoundBase 800

SEQEDIT gSeqEd;
Seq *pSeq = NULL;
Seq *pSeqImport = NULL;

char* pasteDirection[] = {
	
	"&Current frame only",
	"All &next frames",
	"All &previous frames",
	"&All frames",
	
};

CHECKBOX_LIST clipboardMenu[] = {
	
	{FALSE,		"&Tile"},
	{TRUE,		"&X-repeat"},
	{TRUE,		"&Y-repeat"},
	{TRUE,		"&Shade"},
	{TRUE,		"&Palette"},
	{FALSE,		"&Flags"},
	
};

void seqeditStart(char* seqFile) {

	short sect;
	BOOL showMenu = TRUE;
	int i, screenMode = qsetmode;
	
	updatesector(posx, posy, &sect);
	
	i = getHighlightedObject();
	gTool.cantest = (i && sect >= 0 && numsectors > 0);
	gSeqEd.edit3d = (gTool.cantest && screenMode == 200);

	if (gTool.cantest && i != 200)
	{
		if (!Confirm("The object is not a sprite. Spawn SEQ and continue anyway?"))
			return;
	}
	
	
	seqKillAll();
	
	pSeq = (Seq*)Resource::Alloc(kSeqMemSize);
	memset(pSeq, 0, kSeqMemSize);
	pSeq->ticksPerFrame = 12;
	gTool.surface = kSurfStone;

	if (seqFile)
	{
		sprintf(gPaths.seqs, seqFile);
		if ((showMenu = (!SEQLoad(pSeq))) == TRUE)
			Alert("Failed to load SEQ %s", gPaths.seqs);
	}
	
	if (showMenu)
	{
		while( 1 )
		{
			switch (showButtons(toolMenu, LENGTH(toolMenu), gToolNames[kToolSeqEdit].name) - mrUser) {
				case 1:
					if (!toolLoadAs(gSeqEd.filename, kSeqExt) || !SEQLoad(pSeq)) continue;
					break;
				case 0:
					seqeditNewAnim();
					break;
				default:
					return;
			}
			
			break;
		}
	}
	
	toolGetResTableValues();
	toolSetOrigin(&gSeqEd.origin, xdim >> 1, ydim >> 1);
	
	if (gTool.cantest)
	{
		gTool.objType  = (char)searchstat;
		gTool.objIndex = (i == 300) ? searchsector : searchwall;
		
		cpySaveObject(gTool.objType, gTool.objIndex);
		if (gTool.objType == OBJ_SPRITE)
			GetSpriteExtents(&sprite[gTool.objIndex], &gTool.zTop, &gTool.zBot);

		seqeditObjectInit(FALSE);
	}
	
	artedInit(); 	// switch to art editing mode as well
	gArtEd.mode 	= kArtEdModeBatch;
	// only allow batch tile edit mode (seqedit has internal offsets editor)
	gArtEd.modeMin	= gArtEd.mode;
	gArtEd.modeMax	= gArtEd.mode;
	
	gTileView.bglayers++;
	xmpSetEditMode(0x01);

	seqeditProcess();
	
	gTileView.bglayers--;
	
	// finished the editing
	if (gTool.cantest)
		cpyRestoreObject(gTool.objType, gTool.objIndex);
	
	keyClear();
	gSeqEd.clipboardOk = FALSE;
	gTool.objType = -1;
	gTool.objIndex = -1;
	gTool.pSprite = NULL;
	Resource::Free(pSeq);
	pSeq = NULL;

	gScreen.msg[0].time = 0;
	artedUninit();
	
	if (screenMode == 200)		xmpSetEditMode(0x01);
	else						xmpSetEditMode(0x00);

}

void seqeditProcess( void ) {
	
	int timeCounter = 0, fidx = 0, nTile = 0, nTileView = 0, nTileTemp = 0;
	int nOctant = 0, i = 0, j = 0, k = 0, x = kMar, y = kMar, gStep;
	BYTE playing = 0, key, ctrl, shift, alt;
	spritetype* pSprite = gTool.pSprite;
	QFONT* pFont = gTool.pFont;
	char* message = gScreen.msg[0].text;
	
	while ( 1 ) {

		updateClocks();
		sndProcess();

		fidx = ClipRange(fidx, 0, pSeq->nFrames);
		SEQFRAME *pFrame = (pSeq->nFrames <= 0) ? NULL :&pSeq->frames[fidx];

		if (!pFrame)
		{
			gfxSetColor(gStdColor[28]);
			gfxFillBox(0, 0, xdim, ydim);
			toolDrawPixels(gTool.hudPixels, gStdColor[29]);
			sprintf(buffer, "You must insert a new frame"); i = gfxGetTextLen(strupr(buffer), pFont);
			gfxPrinTextShadow((xdim >> 1) - (i >> 1), (ydim >> 1) - (pFont->height >> 1), gStdColor[15], buffer, pFont);
		}
		else
		{
			if (gTool.objIndex >= 0)
			{
				seqeditObjectUpdate(pFrame);
				nOctant = updViewAngle(pSprite);
			}
			
			// inherit palette from previous frames if current is zero
			if (pFrame->pal == 0)
			{
				for (i = fidx, j = 0; i >= 0; i--) {
					if (pSeq->frames[i].pal == 0) continue;
					gSeqEd.curPal = (char)pSeq->frames[i].pal;
					j = 1;
					break;
				}
				
				if (j != 1)
					gSeqEd.curPal = 0;
			}
			else
			{
				gSeqEd.curPal = (char)pFrame->pal;
			}

			if (gSeqEd.edit3d)
			{
				processDrawRooms();
				processMove();
				processMouseLook3D();
				gMouse.Draw();
				
				nTileView = toolGetViewTile(nTile, nOctant, NULL, NULL);
			}
			else
			{
				gfxSetColor(gStdColor[28]);
				gfxFillBox(0, 0, xdim, ydim);
				toolDrawPixels(gTool.hudPixels, gStdColor[29]);
				
				j = ydim / 3;
				toolDrawCenter(&gSeqEd.origin, gStdColor[20], j, j, gTool.centerPixels);
				toolDrawCenterHUD(&gSeqEd.origin, nTile, nTileView, j, nOctant, gStdColor[19]);
				seqeditDrawTile(pFrame, gTool.tileZoom, &nTileView, nOctant);
				toolDrawCenter(&gSeqEd.origin, gStdColor[20], 6, 6, 1);
			}
		}
		
		// title
		if (totalclock > gScreen.msg[0].time)
		{
			
			j = gStdColor[15];
			sprintf(message, "%s", (strlen(gSeqEd.filename) ? gSeqEd.filename : "Unnamed"));
			if (gSeqEd.rffID >= 0)
			{
				strcat(message, " ");
				sprintf(buffer, "(ID: %d)", gSeqEd.rffID);
				strcat(message, buffer);
			}
			
			strcat(message, " ");
			strcat(message, "-");
			strcat(message, " ");

			sprintf(buffer, "%s", gToolNames[kToolSeqEdit].name);
			strcat(message, buffer);
			
			strupr(message);
			
		// message
		}
		else
		{
			j = gStdColor[(gFrameClock & 32) ? 15 : 7];
		}

		toolDrawWindow(0, 0, xdim, ydim, message, (char)j);

		y = 15 + kMar;
		
		if (playing)
		{
			scrSetMessage("NOW PLAYING");
			timeCounter -= gFrameTicks;
			while (timeCounter < 0)
			{
				timeCounter += pSeq->ticksPerFrame;
				if (++fidx == pSeq->nFrames)
				{
					if (playing & 0x02) fidx = 0;
					else gScreen.msg[0].time = fidx = playing = 0;
					sndKillAllSounds();
				}

				if (playing)
				{
					if (pSeq->frames[fidx].playSound)
						playSound(pSeq->nSoundID+pSeq->frames[fidx].soundRange, 0);
					
					if (pSeq->frames[fidx].surfaceSound)
						playSound(kSurfSoundBase+gTool.surface+Random(1), 0);
				}
			}
		}
		
		fidx = ClipRange(fidx, 0, pSeq->nFrames);
		pFrame = (pSeq->nFrames <= 0) ? NULL :&pSeq->frames[fidx];
		sprintf(buffer, "SEQUENCE: TICKS PER FRAME=%03d / SHADE=%03d / SOUND=%05d", pSeq->ticksPerFrame, gTool.shade, pSeq->nSoundID);
		gfxPrinTextShadow(kMar, y, gStdColor[16], buffer, pFont);
		
		buffer[0] = 0;
		if (pSeq->flags & kSeqRemove)
		{	
			sprintf(buffer, "[ DELETE ON ]");
			j = 12;
		}
		else if (pSeq->flags & kSeqLoop)
		{
			sprintf(buffer, "[ LOOP ON ]");
			j = 9;
		}
		
		if (buffer[0] != 0)
		{
			i = gfxGetTextLen(buffer, pFont);
			gfxPrinTextShadow(xdim - kMar - i, y, gStdColor[j], buffer, pFont);
		}

		if (pFrame)
		{
			nTile = seqGetTile(pFrame);

			y = y + pFont->height + 2;
			gfxSetColor(gStdColor[6]);
			gfxHLine(y, kMar, xdim - kMar);
			y+=4;
			
			sprintf(buffer, "VIEW TILE = %04d", nTileView);
			gfxPrinTextShadow(kMar, y, gStdColor[17], strupr(buffer), pFont);
						
			sprintf(buffer, "X-OFFSET = %04d", panm[nTileView].xcenter);
			gfxPrinTextShadow(kMar, y+=pFont->height, gStdColor[17], strupr(buffer), pFont);
			
			sprintf(buffer, "Y-OFFSET = %04d", panm[nTileView].ycenter);
			gfxPrinTextShadow(kMar, y+=pFont->height, gStdColor[17], strupr(buffer), pFont);
			
			sprintf(buffer, "SURFACE = %s", surfNames[surfType[nTileView]]);
			gfxPrinTextShadow(kMar, y+=pFont->height, gStdColor[17], strupr(buffer), pFont);
			
			sprintf(buffer, "VIEW TYPE = %s", viewNames[viewType[nTileView]]);
			gfxPrinTextShadow(kMar, y+=pFont->height, gStdColor[17], strupr(buffer), pFont);
			
			memset(buffer, 0, sizeof(buffer)); sprintf(buffer, "auto");
			if (pFrame->pal) sprintf(&buffer[16], "%03d", pFrame->pal);
			else sprintf(&buffer[16], buffer);
			
			if (pFrame->xrepeat) sprintf(&buffer[32], "%03d", pFrame->xrepeat);
			else sprintf(&buffer[32], buffer);
			
			if (pFrame->yrepeat) sprintf(&buffer[64], "%03d", pFrame->yrepeat);
			else sprintf(&buffer[64], buffer);

			sprintf(buffer2, 
				"TILE=%04d  SHADE=%+04d  PALETTE=%s  X-REPEAT=%s  Y-REPEAT=%s  SOUND RANGE=%02d",
				nTile, pFrame->shade, &buffer[16], &buffer[32], &buffer[64], pFrame->soundRange
			);

			y = ydim - kMar - pFont->height;
			gfxPrinTextShadow(kMar, y, gStdColor[16], strupr(buffer2), pFont);

			y = y - 4;
			gfxSetColor(gStdColor[6]);
			gfxHLine(y, kMar, xdim - kMar);
			y = y - pFont->height - 2;
			
			sprintf(buffer, "FRAME %d OF %d", (pSeq->nFrames > 0) ? fidx + 1 : fidx, pSeq->nFrames);
			gfxPrinTextShadow(kMar, y, gStdColor[16], buffer, pFont);
			
			int yofs = pFont->height + (pFont->height >> 2);
			y = y + pFont->height; x = xdim - (pFont->width << 3) - (kMar << 1); 

			if (pFrame->blockable) 			seqeditPrintFlags(x, y-=yofs, 	"BLOCKING", pFont, 4);
			if (pFrame->invisible)			seqeditPrintFlags(x, y-=yofs, 	"INVISIBLE", pFont, 0, 7);
			if (pFrame->autoaim)			seqeditPrintFlags(x, y-=yofs,   "AUTO-AIM", pFont, 10);
			if (pFrame->hittable)			seqeditPrintFlags(x, y-=yofs, 	"HITSCAN", pFont, 5);
			if (pFrame->smoke)				seqeditPrintFlags(x, y-=yofs, 	"SMOKE", pFont, 6);
			if (pFrame->trigger)			seqeditPrintFlags(x, y-=yofs, 	"TRIGGER", pFont, 12);
			if (pFrame->pushable)			seqeditPrintFlags(x, y-=yofs, 	"PUSHABLE", pFont, 2);
			
			if (pFrame->transparent2)		seqeditPrintFlags(x, y-=yofs, 	"TRANSLUCENT1", pFont, 7, 0);
			else if (pFrame->transparent) 	seqeditPrintFlags(x, y-=yofs, 	"TRANSLUCENT2", pFont, 8, 0);
			
			if (pFrame->surfaceSound) 		seqeditPrintFlags(x, y-=yofs, 	"SURFACE SND", pFont, 15);
			if (pFrame->playSound)			seqeditPrintFlags(x, y-=yofs, 	"PLAY SOUND", pFont, 14);
			if (pFrame->xflip) 				seqeditPrintFlags(x, y-=yofs, 	"FLIP X", pFont, 9);
			if (pFrame->yflip) 				seqeditPrintFlags(x, y-=yofs, 	"FLIP Y", pFont, 13);
		}
		
		showframe();
		handleevents();
		keyGetHelper(&key, &ctrl, &shift, &alt);
		
		switch (key) {
			case KEY_ESC:
			case KEY_SPACE:
				if (!playing) break;
				fidx = gScreen.msg[0].time = playing = 0;
				sndKillAllSounds();
				continue;
			case KEY_PADENTER:
				if (!gTool.cantest) Alert("You must load a map to enable 3D mode.");
				else scrSetMessage("3D edit mode is %s", onOff(gSeqEd.edit3d^=1));
				break;
			case KEY_PADSTAR:
			case KEY_PADSLASH:
				i = (key == KEY_PADSTAR) ? 0x1000 : -0x1000;
				gTool.tileZoom = ClipRange(gTool.tileZoom + i, 0x1000, 0x10000 << 4);
				scrSetMessage("Zoom: %d%%.", (gTool.tileZoom * 100) / toolGetDefTileZoom());
				break;
			case KEY_COMMA:
			case KEY_PERIOD:
			case KEY_SLASH:
				if (gTool.objType == OBJ_SPRITE)
				{
					// set angle for the sprite to keep edit modes in sync
					if (key == KEY_SLASH) pSprite->ang = (short)(ang + kAng180);
					else if (key == KEY_COMMA) pSprite->ang += kAng45;
					else pSprite->ang -= kAng45;
					
					pSprite->ang = (short)(pSprite->ang & kAngMask);
					nOctant = updViewAngle(pSprite);
				}
				else if (key == KEY_SLASH) nOctant = 0;
				else if (key == KEY_COMMA) nOctant = DecRotate(nOctant, 8);
				else nOctant = IncRotate(nOctant, 8);
				break;
			case KEY_PLUS:
				if (ctrl)
				{
					gTool.surface = (BYTE)IncRotate(gTool.surface, LENGTH(surfNames) - 1);
					scrSetMessage("Floor surface type is %s", surfNames[gTool.surface]);
				}
				else
				{
					pSeq->ticksPerFrame = (short)ClipHigh(pSeq->ticksPerFrame + 1, 32767);
					gSeqEd.asksave = TRUE;
				}
				break;
			case KEY_MINUS:
				if (ctrl)
				{
					gTool.surface = (BYTE)DecRotate(gTool.surface, LENGTH(surfNames) - 1);
					scrSetMessage("Floor surface type is %s", surfNames[gTool.surface]);
				}
				else
				{
					pSeq->ticksPerFrame = (short)ClipLow(pSeq->ticksPerFrame - 1, 1);
					gSeqEd.asksave = TRUE;
				}
				break;
				
		}

		if (playing)
			continue;

		switch (key) {
			case KEY_ESC:
				if (!gSeqEd.asksave) return;
				switch (i = DlgSaveChanges("Save sequence?", gArtEd.asksave)) {
					case 2:
					case 1:
						if (!gSeqEd.filename[0] && !toolSaveAs(gSeqEd.filename, kSeqExt)) break;
						if (i > 1) artedSaveChanges();
						SEQSave();
						// no break
					case 0:
						return;
					default:
						break;
				}
				break;
			case KEY_F1:
				if (!Confirm("Start new animation?")) break;
				seqeditNewAnim();
				scrSetMessage("New animation created.");
				break;
			case KEY_F2:
				if (pSeq->nFrames <= 0)
				{
					Alert("Cannot save empty animation.");
					break;
				}
				else if (((!strlen(gSeqEd.filename) || ctrl) && !toolSaveAs(gSeqEd.filename, kSeqExt))) break;
				else
				{
					gSeqEd.asksave = FALSE;
					gSeqEd.rffID = -1;
					SEQSave();
					if (gArtEd.asksave && Confirm("Save ART changes?"))
						artedSaveChanges();
					BeepOk();
				}
				break;
			case KEY_F11: // load previous existing fileID
			case KEY_F12: // load next existing fileID
				i = getClosestId(ClipRange(gSeqEd.rffID, 0, 65534), 65534, gExtNames[kSeq], (BOOL)(key == KEY_F12));
				sprintf(gSeqEd.filename, "%d", i);
				if (SEQLoad(pSeq))
				{
					gSeqEd.asksave = FALSE; gSeqEd.rffID = i; fidx = 0;
					seqeditObjectInit(TRUE);
					seqeditObjectUpdate(&pSeq->frames[fidx]);
					BeepOk();
				}
				break;
			case KEY_F3:
				if (!toolLoadAs(gSeqEd.filename, kSeqExt) || !SEQLoad(pSeq)) break;
				gSeqEd.asksave = FALSE;
				BeepOk();
				fidx = 0;
				break;
			case KEY_F4:// import seq animation
				j = gSeqEd.rffID;
				sprintf(buffer, gSeqEd.filename);
				if (toolLoadAs(gSeqEd.filename, kSeqExt, "Import SEQ file") != NULL)
				{
					pSeqImport = (Seq*)Resource::Alloc(kSeqMemSize);
					memset(pSeqImport, 0, kSeqMemSize);
					
					if (SEQLoad(pSeqImport))
					{
						for (i = 0, k = pSeq->nFrames; i < pSeqImport->nFrames; i++, k++)
						{
							if (k >= kSeqMaxFrames)
							{
								Alert("Max (%d) frames reached!", kSeqMaxFrames);
								break;
							}
							
							pSeq->frames[k] = pSeqImport->frames[i];
							pSeq->nFrames = (short)k;
						}

						Alert("%d of %d frames imported successfully.", i, pSeqImport->nFrames);
					}
					else
					{
						Alert("Failed to import \"%s\"!", gSeqEd.filename);
					}
					
					Resource::Free(pSeqImport);
					pSeqImport = NULL;
					
				}
				sprintf(gSeqEd.filename, buffer);
				gSeqEd.rffID = j;
				gSeqEd.asksave = TRUE;
				break;
			case KEY_SPACE:
				if (pSeq->nFrames < 1 || pSeq->ticksPerFrame <= 0) break;
				playing = (shift) ? 0x01 : 0x02;
				timeCounter = fidx = 0;
				break;
			case KEY_F10:
				playSound(pSeq->nSoundID, (ctrl) ? 0 : pSeq->frames[fidx].soundRange);
				break;
			case KEY_1:
			case KEY_2:
				if (pSeq->nFrames <= 0) {
					Alert("You must insert frame.");
					break;
				}
				switch (key){
					case KEY_1:
						if (fidx > 0) fidx--;
						else scrSetMessage("First frame reached.");
						break;
					case KEY_2:
						if (fidx < pSeq->nFrames - 1) fidx++;
						else scrSetMessage("Last frame reached.");
						break;
				}
				break;
			case KEY_HOME:
			case KEY_END:
				fidx = (key == KEY_HOME) ? 0 : pSeq->nFrames - 1;
				break;
			case KEY_G:
				fidx = ClipRange(GetNumberBox("Goto frame", 1, 1), 0, pSeq->nFrames - 1);
				break;
			case KEY_V:
				if (fidx >= pSeq->nFrames) break;
				nTileTemp = tilePick(nTile, nTile, OBJ_ALL, "Frame picnum");
				if (pSeq->nFrames <= 0) {
					seqeditSetFrameDefaults(pFrame);
					pSeq->nFrames++;
				}
				seqFrameSetTile(pFrame, nTileTemp);
				gSeqEd.asksave = TRUE;
				break;
			case KEY_INSERT:
				if ((nTileTemp = tilePick(nTile, -1, OBJ_ALL, "Insert new frame")) < 0) break;
				else if (pSeq->nFrames >= kSeqMaxFrames)
				{
					Alert("Max (%d) frames reached!", kSeqMaxFrames);
					break;
				} 
				else if (pSeq->nFrames <= 0) // insert first frame
				{
					seqeditSetFrameDefaults(&pSeq->frames[fidx]);
				} 
				else
				{
					if (!ctrl) fidx++; // insert next frame AFTER current (BEFORE otherwise)
					for (i = pSeq->nFrames; i >= fidx && i - 1 >= 0; i--)
						pSeq->frames[i] = pSeq->frames[i - 1];
				}
				pSeq->frames[fidx].playSound = 0;
				pSeq->frames[fidx].surfaceSound = 0;
				pSeq->frames[fidx].soundRange = 0;
				seqFrameSetTile(&pSeq->frames[fidx], nTileTemp);
				pSeq->nFrames++; gSeqEd.asksave = TRUE;
				scrSetMessage("New frame created.");
				BeepOk();
				break;
			case KEY_DELETE:
				if (pSeq->nFrames <= 0) break;
				memmove(pFrame, &pSeq->frames[fidx + 1], sizeof(SEQFRAME) * (pSeq->nFrames - fidx - 1));
				if (fidx == --pSeq->nFrames) fidx--;
				gSeqEd.asksave = TRUE;
				scrSetMessage("Frame deleted.");
				BeepOk();
				break;
		}

		switch (key) {
			case KEY_F5: 		case KEY_F6:
			case KEY_X:			case KEY_Y:			case KEY_M:
			case KEY_B:			case KEY_I:			case KEY_T:
			case KEY_H:			case KEY_K:			case KEY_U:
			case KEY_L:			case KEY_D:			case KEY_R:
			case KEY_S:			case KEY_P:			case KEY_F9:
			case KEY_PADMINUS:	case KEY_PADPLUS:	case KEY_PAD0:
			case KEY_PADUP:		case KEY_PADDOWN:	case KEY_PADLEFT:
			case KEY_PADRIGHT:	case KEY_PAD7:		case KEY_PAD9:
			case KEY_PAGEUP:	case KEY_PAGEDN:
				if (fidx >= pSeq->nFrames) continue;
				gSeqEd.asksave = TRUE;
				break;
		}

		switch (key) {
			case KEY_X: pFrame->xflip 			= !pFrame->xflip; 			break;
			case KEY_Y: pFrame->yflip 			= !pFrame->yflip; 			break;
			case KEY_M: pFrame->autoaim 		= !pFrame->autoaim; 		break;
			case KEY_B: pFrame->blockable 		= !pFrame->blockable; 		break;
			case KEY_I: pFrame->invisible 		= !pFrame->invisible; 		break;
			case KEY_H: pFrame->hittable 		= !pFrame->hittable; 		break;
			case KEY_K:	pFrame->smoke 			= !pFrame->smoke; 			break;
			case KEY_U: pFrame->surfaceSound 	= !pFrame->surfaceSound; 	break;
			case KEY_T: pFrame->trigger 		= !pFrame->trigger; 		break;
			case KEY_R:
				if (!pFrame->transparent) pFrame->transparent = 1;
				else if (!pFrame->transparent2) pFrame->transparent2 = 1;
				else pFrame->transparent = pFrame->transparent2 = 0;
				break;
			case KEY_F5: {
				sprintf(buffer, "Tile#%d view type", nTile);
				if ((i = showButtons(viewNames, LENGTH(viewNames), buffer)) < mrUser) break;
				viewType[nTile] = (BYTE)(i - mrUser);
				if (!isExternalModel(nTile))
					panm[nTile].view = viewType[nTile];
				artedArtDirty(nTile, kDirtyPicanm);
				gSeqEd.asksave = TRUE;
				break;
			}
			case KEY_F6:
				sprintf(buffer, "Tile#%d surface", nTileView);
				if ((i = showButtons(surfNames, LENGTH(surfNames), buffer)) < mrUser) break;
				surfType[nTileView] = (BYTE)(i - mrUser);
				artedArtDirty(nTileView, kDirtyDat);
				gSeqEd.asksave = TRUE;
				break;
			case KEY_F9: // reverse frames order
			{
				SEQFRAME cpyframe;
				for (i = 0; i < pSeq->nFrames >> 1; i++)
				{
					cpyframe = pSeq->frames[i];
					pSeq->frames[i] = pSeq->frames[pSeq->nFrames - i - 1];
					pSeq->frames[pSeq->nFrames - i - 1] = cpyframe;
				}
				scrSetMessage("Reverse frames order.");
				break;
			}
			case KEY_L:
				pSeq->flags ^= kSeqLoop;
				pSeq->flags &= ~kSeqRemove;
				break;
			case KEY_D:
				pSeq->flags ^= kSeqRemove;
				pSeq->flags &= ~kSeqLoop;
				break;
			case KEY_TAB:
				memcpy(&gSeqEd.clipboard, pFrame, sizeof(SEQFRAME));
				scrSetMessage("Copy frame #%d.", fidx + 1);
				gSeqEd.clipboardOk = TRUE;
				BeepOk();
				break;
			case KEY_ENTER:
				if (!gSeqEd.clipboardOk)
				{
					scrSetMessage("There is nothing to paste.");
					BeepFail();
					break;
				}
				
				gSeqEd.asksave = TRUE;
				if (!shift) // paste whole frame
				{
					memcpy(pFrame, &gSeqEd.clipboard, sizeof(SEQFRAME));
					scrSetMessage("Paste frame #%d", fidx + 1);
					BeepOk();
					break;
				}
				else	// paste selected properties
				{ 
					while ( 1 )
					{
						if ((i = createCheckboxList(clipboardMenu, LENGTH(clipboardMenu), "Paste properties", TRUE)) == 0) break;
						else if ((j = showButtons(pasteDirection, LENGTH(pasteDirection), "Select direction")) < mrUser)
							continue;
							
						switch (j-=mrUser) {
							case 0:		i = fidx;		break;
							case 1:		i = fidx + 1;	break;
							case 2:		i = fidx - 1;	break;
							case 3:		i = 0; 			break;
						}
						
						i = ClipRange(i, 0, pSeq->nFrames - 1);
						
						while ( 1 ) {
							
							if (clipboardMenu[0].option)
							{
								pSeq->frames[i].tile  = gSeqEd.clipboard.tile;
								pSeq->frames[i].tile2 = gSeqEd.clipboard.tile2;
							}
							if (clipboardMenu[1].option) pSeq->frames[i].xrepeat = gSeqEd.clipboard.xrepeat;
							if (clipboardMenu[2].option) pSeq->frames[i].yrepeat = gSeqEd.clipboard.yrepeat;
							if (clipboardMenu[3].option) pSeq->frames[i].shade = gSeqEd.clipboard.shade;
							if (clipboardMenu[4].option) pSeq->frames[i].pal = gSeqEd.clipboard.pal;
							if (clipboardMenu[5].option)
							{
								pSeq->frames[i].xflip 			= gSeqEd.clipboard.xflip;
								pSeq->frames[i].yflip 			= gSeqEd.clipboard.yflip;
								pSeq->frames[i].autoaim 		= gSeqEd.clipboard.autoaim;
								pSeq->frames[i].blockable 		= gSeqEd.clipboard.blockable;
								pSeq->frames[i].invisible 		= gSeqEd.clipboard.invisible;
								pSeq->frames[i].hittable 		= gSeqEd.clipboard.hittable;
								pSeq->frames[i].smoke 			= gSeqEd.clipboard.smoke;
								pSeq->frames[i].surfaceSound 	= gSeqEd.clipboard.surfaceSound;
								pSeq->frames[i].trigger 		= gSeqEd.clipboard.trigger;
								pSeq->frames[i].transparent		= gSeqEd.clipboard.transparent;
								pSeq->frames[i].transparent2	= gSeqEd.clipboard.transparent2;
								pSeq->frames[i].pushable			= gSeqEd.clipboard.pushable;
								pSeq->frames[i].playSound		= gSeqEd.clipboard.playSound;
							}
							switch (j) {
								case 1:
								case 3:
									if (++i >= pSeq->nFrames) break;
									continue;
								case 2:
									if (--i < 0) break;
									continue;
							}
							break;
							
						}
						
						scrSetMessage("Paste selected properties.");
						BeepOk();
						break;
						
					}

				}
				break;
			case KEY_S:
				if (alt || pSeq->nSoundID <= 0)
					pSeq->nSoundID = (short)GetNumberBox("Global Sound ID", pSeq->nSoundID, pSeq->nSoundID);
				if (pSeq->nSoundID > 0 && ctrl && !alt)
					pFrame->soundRange = ClipRange(GetNumberBox("Random Sound Range", pFrame->soundRange, pFrame->soundRange), 0, 15);
				if (pSeq->nSoundID > 0 && !ctrl && !alt)
					pFrame->playSound = !pFrame->playSound;
				break;
			case KEY_P:
				gSeqEd.asksave = TRUE;
				if (!alt && !shift)
				{
					pFrame->pushable = !pFrame->pushable;
					break;
				}
				else if (alt) pFrame->pal = GetNumberBox("Palookup", pFrame->pal, pFrame->pal);
				else if (shift & 0x01) pFrame->pal = getClosestId(pFrame->pal, kPluMax - 1, "PLU", TRUE);
				else if (shift & 0x02) pFrame->pal = getClosestId(pFrame->pal, kPluMax - 1, "PLU", FALSE);	
				scrSetMessage("Frame #%d palookup: %d", fidx, pFrame->pal);
				break;
			case KEY_PADMINUS:
			case KEY_PADPLUS:
				if (shift) {
					gTool.shade = (schar) ((key == KEY_PADMINUS) ? ClipHigh(gTool.shade + 1, 127) : ClipLow(gTool.shade - 1, -128));
					gSeqEd.asksave = FALSE;
					break;
				} else if (alt) {
					pFrame->shade = GetNumberBox("Frame shade", pFrame->shade, pFrame->shade);
					break;
				}
				switch (key) {
					case KEY_PADMINUS:
						if (ctrl) pFrame->shade = 127;
						else pFrame->shade = ClipHigh(pFrame->shade + 1, 127);
						break;
					case KEY_PADPLUS:
						if (ctrl) pFrame->shade = -128;
						else pFrame->shade = ClipLow(pFrame->shade - 1, -128);
						break;
				}
				break;
			case KEY_PAD0:
				if (!ctrl) pFrame->shade = 0;
				else gTool.shade = 0;
				break;
			case KEY_PADUP:
			case KEY_PADDOWN:
			case KEY_PAD7:
			case KEY_PAD9:
				gStep = shift ? 1 : 4;
				if (key != KEY_PAD7 && key != KEY_PAD9 && !ctrl && alt)
				{
					pFrame->yrepeat = GetNumberBox("Frame Y-Repeat", pFrame->yrepeat, pFrame->yrepeat);
					break;
				}
				switch (key) {
					case KEY_PADDOWN:
					case KEY_PAD7:
						if (ctrl) pFrame->yrepeat = 0;
						else pFrame->yrepeat = DecNext(pFrame->yrepeat, gStep);
						break;
					case KEY_PADUP:
					case KEY_PAD9:
						if (ctrl) pFrame->yrepeat = 255;
						else pFrame->yrepeat = IncNext(pFrame->yrepeat, gStep);
						break;
				}
				
				if (key != KEY_PAD7 && key != KEY_PAD9) break;
				// no break
			case KEY_PADLEFT:
			case KEY_PADRIGHT:
				gStep = shift ? 1 : 4;
				if (key != KEY_PAD7 && key != KEY_PAD9 && !ctrl && alt)
				{
					pFrame->xrepeat = GetNumberBox("Frame X-Repeat", pFrame->xrepeat, pFrame->xrepeat);
					break;
				}
				switch (key) {
					case KEY_PADLEFT:
					case KEY_PAD7:
						if (ctrl) pFrame->xrepeat = 0;
						else pFrame->xrepeat = DecNext(pFrame->xrepeat, gStep);
						break;
					case KEY_PADRIGHT:
					case KEY_PAD9:
						if (ctrl) pFrame->xrepeat = 255;
						else pFrame->xrepeat = IncNext(pFrame->xrepeat, gStep);
						break;
				}
				break;
			case KEY_PAGEUP:
			case KEY_PAGEDN:
				nTileTemp = seqGetTile(pFrame);
				while ( 1 ) {
					
					nTileTemp = (key == KEY_PAGEDN) ? nTileTemp - 1 : nTileTemp + 1;
					if (nTileTemp < 0 || nTileTemp >= kMaxTiles) break;
					else if (!tilesizx[nTileTemp] || !tilesizy[nTileTemp])
						continue;
					
					seqFrameSetTile(pFrame, nTileTemp);
					break;
					
				}
				break;
			case KEY_UP:
			case KEY_DOWN:
			case KEY_RIGHT:
			case KEY_LEFT:
			case KEY_PAD5: {
				if (gSeqEd.edit3d || fidx >= pSeq->nFrames) break;
				switch (key) {
					case KEY_UP:
						if (!shift) panm[nTileView].ycenter = ClipHigh(panm[nTileView].ycenter + 1, 127);
						else panm[nTileView].ycenter = ClipHigh(tilesizy[nTileView] - tilesizy[nTileView] / 2, 127);
						break;
					case KEY_DOWN:
						if (!shift) panm[nTileView].ycenter = ClipLow(panm[nTileView].ycenter - 1, -128);
						else panm[nTileView].ycenter = ClipLow(-tilesizy[nTileView] / 2, -128);
						break;
					case KEY_LEFT:
						if (!shift) panm[nTileView].xcenter = ClipHigh(panm[nTileView].xcenter + 1, 127);
						else panm[nTileView].xcenter = ClipHigh(-tilesizx[nTileView] / 2, 127);
						break;
					case KEY_RIGHT:
						if (!shift) panm[nTileView].xcenter = ClipLow(panm[nTileView].xcenter - 1, -128);
						else panm[nTileView].xcenter = ClipLow(tilesizx[nTileView] - tilesizx[nTileView] / 2, -128);
						break;
					case KEY_PAD5:
						panm[nTileView].xcenter = 0;
						panm[nTileView].ycenter = 0;
						break;
				}
				artedArtDirty(nTileView, kDirtyPicanm);
				gSeqEd.asksave = TRUE;
				break;
			}
		}
		
		keyClear();

	}
}

void seqeditNewAnim() {
	
	if (pSeq != NULL) {
		Resource::Free(pSeq);
		pSeq = NULL;
	}
	
	if (pSeq == NULL)
	{
		pSeq = (Seq *)Resource::Alloc(kSeqMemSize);
		memset(pSeq, 0, kSeqMemSize);
		pSeq->ticksPerFrame = 12;
	}
	
	gSeqEd.rffID = -1;
	pSeq->ticksPerFrame = 12;
	gSeqEd.filename[0] = 0;
	gSeqEd.asksave = FALSE;
	
	
}

BOOL SEQSave( void ) {
	
	int hFile;
	if (gSeqEd.filename[0] == 0)
		return FALSE;

	ChangeExtension(gSeqEd.filename, getExt(kSeq));
	if ((hFile = open(gSeqEd.filename, O_CREAT|O_WRONLY|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE)) == -1)
	{
		Alert("Error creating SEQ file");
		return FALSE;
	}

	sprintf(pSeq->signature, kSEQSig); pSeq->version = 0x0301;
	write(hFile, pSeq, sizeof(Seq) + pSeq->nFrames * sizeof(SEQFRAME));
	scrSetMessage("Sequence saved.");
	gSeqEd.asksave = FALSE;
	close(hFile);
	return TRUE;
	
}

BOOL SEQLoad(Seq* out) {
	
	dassert(out != NULL);
	
	int i, hFile;
	DICTNODE* hSeq = NULL;
	if (gSeqEd.filename[0] == 0)
		return FALSE;
		
	ChangeExtension(gSeqEd.filename, getExt(kSeq));
	i = fileExists(gSeqEd.filename, &hSeq);
		
	// first try load from the disk
	if ((i & 0x1) && (hFile = open(gSeqEd.filename, O_RDONLY|O_BINARY, S_IWRITE|S_IREAD)) >= 0)
	{
		read(hFile, out, ClipHigh(filelength(hFile), kSeqMemSize));
		gSeqEd.rffID = -1;
		close(hFile);
	}
	else if (hSeq != NULL) // then load from rff
	{
		Seq* pRFFSeq = (Seq*)gSysRes.Load(hSeq);
		memcpy(out, pRFFSeq, ClipHigh(gSysRes.Size(hSeq), kSeqMemSize));
		sprintf(gSeqEd.filename, hSeq->name);
		gSeqEd.rffID = hSeq->id;
	}
	
	
	if (i > 0 && memcmp(out->signature, kSEQSig, sizeof(out->signature)) == 0)
	{
		gSeqEd.asksave = FALSE;
		scrSetMessage("File %s loaded from the %s.", gSeqEd.filename, (gSeqEd.rffID >= 0) ? "rff" : "disk");
		for (i = 0; i < out->nFrames; i++)
			tilePreloadTile((short)seqGetTile(&out->frames[i]), -2);

		return TRUE;

	}
	
	scrSetMessage("Error loading SEQ %s", gSeqEd.filename);
	return FALSE;

}

void seqeditObjectInit(BOOL cpy) {
	
	if (!gTool.cantest)
		return;

	if (cpy)
		cpyRestoreObject(gTool.objType, gTool.objIndex);
	
	switch (gTool.objType) {
		case OBJ_SPRITE:
			gTool.pSprite = &sprite[gTool.objIndex];
			if ((gTool.pSprite->cstat & kSprRelMask) == kSprFace)
				gTool.pSprite->ang = (short)((ang + kAng180) & kAngMask);
			gTool.pSprite->pal = 0;
			chgSpriteZ(gTool.pSprite, gTool.zBot);
			gTool.surface = surfType[sector[gTool.pSprite->sectnum].floorpicnum];
			break;
		case OBJ_FLOOR:
			sector[gTool.objIndex].floorpal = 0;
			break;
		case OBJ_CEILING:
			sector[gTool.objIndex].ceilingpal = 0;
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			wall[gTool.objIndex].pal = 0;
			break;
	}
	
}

void seqeditObjectUpdate(SEQFRAME* pFrame) {
	
	switch (gTool.objType) {
		case OBJ_SPRITE:
			UpdateSprite(GetXSprite(gTool.objIndex), pFrame);
			chgSpriteZ(gTool.pSprite, gTool.zBot);
			break;
		case OBJ_FLOOR:
			UpdateFloor(GetXSector(gTool.objIndex), pFrame);
			break;
		case OBJ_CEILING:
			UpdateCeiling(GetXSector(gTool.objIndex), pFrame);
			break;
		case OBJ_WALL:
			UpdateWall(GetXWall(gTool.objIndex), pFrame);
			break;
		case OBJ_MASKED:
			UpdateMasked(GetXWall(gTool.objIndex), pFrame);
			break;
	}
	
}

void seqFrameSetTile(SEQFRAME* pFrame, int nTile) {
	
	pFrame->tile = nTile&4095;
	pFrame->tile2 = nTile>>12;
	
}

void seqeditSetFrameDefaults(SEQFRAME* pFrame) {
	
	if (!pFrame)
		return;
	
	pFrame->shade = -4;
	pFrame->transparent2 = 0;
	pFrame->transparent = 0;
	pFrame->blockable = 1;
	pFrame->hittable = 1;
	pFrame->pal = 0;
	
	
}

void seqeditDrawTile(SEQFRAME *pFrame, int zoom, int *nTileArg, int nOctant)
{
	
	char flags = 0;	// default position by origin
	int nTile = seqGetTile(pFrame); int ang = 0;	
	schar nShade = (schar)ClipRange(gTool.shade + pFrame->shade, -128, 127);
	
	if (keystatus[KEY_CAPSLOCK])		flags |= kRSNoMask;
	if (pFrame->transparent)			flags |= kRSTransluc;
	if (pFrame->transparent2)			flags |= kRSTranslucR;
	if (pFrame->xflip)					ang += kAng180, flags ^= kRSYFlip;
	if (pFrame->yflip)					flags |= kRSYFlip;

	nTile = toolGetViewTile(nTile, nOctant, &flags, &ang);

	//origin.x = ClipLow(origin.x, w + panm[nTile].xcenter);
	//origin.x = ClipHigh(origin.x, x - w + panm[nTile].xcenter);
	//origin.y = ClipLow(origin.y, tilesizy[nTile] / 2 + panm[nTile].ycenter);
	//origin.y = ClipHigh(origin.y, y - h + panm[nTile].ycenter);

	if ( !pFrame->invisible )
		rotatesprite(gSeqEd.origin.x << 16, gSeqEd.origin.y << 16, zoom, ang, nTile, nShade,
			(char)gSeqEd.curPal, flags, 0, 0, xdim-1, ydim-1);
	
	*nTileArg = nTile;
}

void seqeditPrintFlags(int x, int y, char* buff, QFONT* pFont, int bcol, int tcol) {
	
	int len = gfxGetTextLen(buff, pFont);
	int mid = (xdim - kMar - 1) - (x - 1)>> 1;

	gfxSetColor(gStdColor[bcol]);
	gfxFillBox(x - 1, y - 1, xdim - kMar, y + pFont->height);
	gfxDrawText((x + mid) - (len >> 1), (y + (pFont->height >> 3)) - 1, gStdColor[tcol], buff, pFont);


}