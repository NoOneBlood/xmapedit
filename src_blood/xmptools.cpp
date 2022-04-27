/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions required for intergrated editors such as artedit or seqedit.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "common_game.h"
#include "editor.h"
#include "replace.h"
#include "keyboard.h"
#include "xmpstub.h"
#include "gui.h"
#include "tile.h"
#include "screen.h"
#include "eventq.h"
#include "gameutil.h"
#include "edit2d.h"
#include "aadjust.h"
#include "xmpfx.h"
#include "xmpgib.h"
#include "xmpconf.h"
#include "xmptools.h"
#include "fire.h"
#include "xmpexplo.h"

#include "xmpmisc.h"
#include "img2tile.h"

XMPTOOL gTool;

int toolRestable[][5] = {

	{200,	2,	4,	3,	0x08B00},
	{480,	4,	6,	1,	0x15500},
	{600,	6,	8,	5,	0x17500},
	{768,	8,	10,	5,	0x22000},
	
};

char* toolMenu[] = {
	
	"&New animation",
	"&Load animation",
	"&Quit",
	
};

int toolOpenWith(char* filename, char flags)
{
	RESHANDLE hFile;
	NAMED_TYPE editors[LENGTH(gExtNames)];
	char tmp[_MAX_PATH], tmp2[_MAX_PATH]; char *dsk, *dir, *fname, *ext;
	int i = 0, j = 0, k = 0, edit = -1;
	
	pathSplit2(filename, tmp, &dsk, &dir, &fname, &ext);
	if (ext[0]) // extension found
	{
		if (fileExists(filename, &hFile))
		{
			// remove dot
			if (ext[0] == '.')
				ext=&ext[1];
			
			// try to auto-detect editor
			for (i = 0; i < LENGTH(gExtNames); i++)
			{
				if (stricmp(ext, gExtNames[i]) == 0)
					return i;
			}
		
		}
		// editor for this extension is not found
		return -1;
	}
	
	// search for files with same name but different extensions...
	for (i = 0; i < LENGTH(gExtNames); i++)
	{
		_makepath(tmp2, dsk, dir, fname, gExtNames[i]);
		if ((k = fileExists(tmp2, &hFile)) <= 0) continue;
		else if (!(k & 0x01) && i == kToolArtEdit)
			continue; // can only work with files outside RFF
		
		editors[j++] = gToolNames[i];  // only show actual editors
		edit = i;
	}
	
	if (j == 1)
		return edit; // found an editor

	if (j > 1)
	{
		// two or more editors can open this file: show dialog
		if ((i = showButtons(editors, j, "Open file with..") - mrUser) < 0) return -2; // aborting
		return i;
	}
	
	// found nothing
	return -1;
	
}


void cpySaveObject(int type, int idx) {
	
	switch (type) {
		case OBJ_SPRITE:
			cpysprite[idx] = sprite[idx];
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			cpysector[idx] = sector[idx];
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			cpywall[idx] = wall[idx];
			break;
	}
	
}

void cpyRestoreObject(int type, int idx) {
	
	switch (type) {
		case OBJ_SPRITE:
			sprite[idx] = cpysprite[idx];
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			sector[idx] = cpysector[idx];
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			wall[idx] = cpywall[idx];
			break;
	}
	
}


BOOL toolSaveAs(char* path, char* ext) {
	return (dirBrowse("Save file as", path, ext, kDirExpTypeSave, 0) != NULL);
}

BOOL toolLoadAs(char* path, char* ext, char* title) {
	return (dirBrowse(title, path, ext) != NULL);
}

int toolAskSaveChanges(char* text) {
	return YesNoCancel(text);
}

void toolGetResTableValues() {
	
	// use restable for hud properties
	for (int i = LENGTH(toolRestable) - 1; i >= 0; i--) {
		if (ydim < toolRestable[i][0]) continue;
		gTool.hudPixels 	= (ushort)toolRestable[i][1];
		gTool.centerPixels  = (ushort)toolRestable[i][2];
		gTool.pFont 	 	= qFonts[toolRestable[i][3]];
		gTool.tileZoom      = toolRestable[i][4];
		break;
	}
}


int toolGetDefTileZoom() {
	
	for (int i = LENGTH(toolRestable) - 1; i >= 0; i--) {
		if (ydim < toolRestable[i][0]) continue;
		return toolRestable[i][4];
	}
	
	return 0x10000;
}


int toolGetViewTile(int nTile, int nOctant, char *flags, int *ang) {
	
	int nTileView = nTile;
	
	switch (viewType[nTile]) {
		case kSprViewFull5:
		case kSprViewFull8:
			if (viewType[nTile] == kSprViewFull8) nTileView += nOctant;
			else if (nOctant <= 4) nTileView += nOctant;
			else
			{
				nTileView += (8 - nOctant);
				if (flags) *flags ^= kRSYFlip;
				if (ang) *ang += kAng180;
			}
			break;
	}
	
	return nTileView;
}

void toolDrawWindow(int x1, int y1, int x2, int y2, char* title, char textColor) {
	
	int i, n = 0;
	int width = x2 - x1;
	int height = y2 - y1;
	int size1 = 1, size2 = -1, size3 = -1;

	char c1 = 22;
	char c2 = 20;

	// draw title
	gfxSetColor(gStdColor[1]);
	gfxFillBox(x1 + 1, y1 + 1, x2 - 1, y1 + 13);
	DrawBevel(x1 + 1,  y1 + 1, x2 - 1, y1 + 14, gStdColor[9], gStdColor[29]);


	for (i = klabs(size1); i > 0; n++, i--)
	{
		if (size1 > 0)
			DrawBevel(x1 + n, y1 + n, x1 + width - n, y1 + height - n, gStdColor[c1], gStdColor[c2]);
		else
			DrawBevel(x1 + n, y1 + n, x1 + width - n, y1 + height - n, gStdColor[c2], gStdColor[c1]);
	}

	n += size2;

	for (i = klabs(size3); i > 0; n++, i--)
	{
		if (size3 > 0)
			DrawBevel(x1 + n, y1 + n, x1 + width - n, y1 + height - n, gStdColor[c1], gStdColor[c2]);
		else
			DrawBevel(x1 + n, y1 + n, x1 + width - n, y1 + height - n, gStdColor[c2], gStdColor[c1]);
	}

	if (title)
	{
		int len = gfxGetTextLen(title, pFont);
		gfxDrawText(x1+((width>> 1)- (len>>1)), y1+(14>>1)-(pFont->height>>1), textColor, title, pFont);
	}
	
}

void toolSetOrigin(POINT* origin, int x, int y )
{
	origin->x = ClipRange(x, 0, xdim - 1);
	origin->y = ClipRange(y, 0, ydim - 1);
}

void toolDrawCenter(POINT* origin, int nColor, int xrng, int yrng, int skip) {
	
	int i; 
	gfxSetColor(nColor);
	for (i = origin->x - xrng; i < origin->x; i+=skip)  gfxPixel(i, origin->y);
	for (i = origin->x + xrng; i >= origin->x; i-=skip) gfxPixel(i, origin->y);
	for (i = origin->y - yrng; i < origin->y; i+=skip)  gfxPixel(origin->x, i);
	for (i = origin->y + yrng; i >= origin->y; i-=skip) gfxPixel(origin->x, i);

	
}

void toolDrawCenterHUD(POINT* origin, int nTile, int nVTile, int scrYofs, int nOctant, int nColor) {
	
	
	int y, i;
	char color = (char)nColor;
	QFONT* pFont = gTool.pFont;
	int ph = pFont->height;

	sprintf(buffer, "X: %+03d", panm[nVTile].xcenter); i = gfxGetTextLen(strupr(buffer), pFont);
	gfxPrinTextShadow(origin->x + scrYofs + (ph << 1), origin->y - (ph >> 1), color, buffer, pFont);

	y = origin->y + scrYofs + (ph << 1);
	sprintf(buffer, "Y: %+03d", panm[nVTile].ycenter); i = gfxGetTextLen(strupr(buffer), pFont);
	gfxPrinTextShadow(origin->x - (i >> 1), y, color, buffer, pFont);

	switch (viewType[nTile]) {
		case kSprViewFull5:
		case kSprViewFull8:
			y += (ph + 2);
			sprintf(buffer, "ANGLE: %0ddeg", nOctant * 45); i = gfxGetTextLen(buffer, pFont);
			gfxPrinTextShadow(origin->x - (i >> 1), y, color, buffer, pFont);
			break;
	}
	
}

void toolDrawPixels(int dist, int color) {
	
	int j, i;
	gfxSetColor(color);
	for (j = dist; j < ydim - dist; j+=dist) {
		for (i = dist; i < xdim - dist; i+=dist) {
			gfxPixel(i, j);
		}
	}
	
}


void toolDisplayMessage(short nColor, int x, int y, QFONT* pFont) {
	if (totalclock < messageTime + 120)
		gfxPrinTextShadow(x, y, nColor, strupr(message), pFont);
}

NAMED_TYPE gibNames[] = {

	{0,  "Glass (transparent)"},
	{1,  "Glass (stained)"},
	{12, "Glass combo 1"},
	{13, "Glass combo 2"},
	{2,  "Burn shard"},
	{3,  "Wood shard"},
	{14, "Wood combo"},
	{4,  "Metal shard"},
	{5,  "Fire spark"},
	{6,  "Electric spark"},
	{17, "Flare spark"},
	{7,  "Blood chunks"},
	{18, "Blood bits"},
	{8,  "Bubbles (small)"},
	{9,  "Bubbles (medium)"},
	{10, "Bubbles (large)"},
	{11, "Icicles"},
	{19, "Rock shard"},
	{20, "Paper combo"},
	{21, "Plant combo"},
	{16, "Mixed combo"},
	{22, "Eletric gibs 1"},
	{23, "Electric gibs (floor)"},
	{24, "Electric gibs 3"},
	{25, "Flames 1"},
	{26, "Flames 2"},
	{27, "Axe zombie head"},
	{15, "Human body parts"},
	{28, "Mime body parts"},
	{29, "Hound body parts"},
	{30, "Gargoyle body parts"},
};



int toolGibTool(int nGib, int type) {

	nGib = 0;
	const int listLen = LENGTH(gibNames); char ids[listLen][4], even = 0;
	memset(ids, 0, 4*listLen);

	short tile, xr, yr, plu;
	int i, j, x, y, tx, ty, len, start;
	int tsize = 28, boxsize = tsize - 4, dwidth = 250, dheigh = 390, page = 0, perpage = 8;
	int shapew = dwidth - 14, shapeh = pFont->height + tsize + 4;
	int bsize = shapeh - 4;

	i = listLen / perpage; int pages = (i * perpage < listLen) ? i + 1 : i;	
	while ( 1 ) {
		
		start = page*perpage;
		x = 4, y = 4, tx = 4, ty = 4;
		
		Window dialog(0, 0, dwidth, ydim - 1, "Gib list");
		dialog.height = dheigh = ((shapeh)*perpage) + (4 * perpage) + 8;
		
		for (i = start; i < start + perpage; i++) {

			x = 4;
			dialog.Insert(new Shape(x, y, shapew, shapeh, gStdColor[8])); // border
			dialog.Insert(new Shape(x + 1, y + 1, shapew - 2, shapeh - 2, gStdColor[20 + (even^=1)])); // fill
			dialog.Insert(new Shape(x + bsize + 3, y, 1, shapeh, gStdColor[8])); // separator
			y += 4; 

			TextButton* pData = NULL; Label* pName = NULL;

			if (i < listLen) {

				GIBFX* gf = NULL; GIBTHING* gt = NULL;
				GIBLIST* gib = &gibList[gibNames[i].id];
				
				len = 0;
				if (gib->at0)
				{
					gf = gib->at0;
					len = gib->at4;
				}
				else if (type != OBJ_WALL && gib->at8)
				{
					gt = gib->at8;
					len = gib->atc;
				}
				
				if (len) {
					
					// button with data value
					j = gibNames[i].id + 1; sprintf(ids[i], "%02d", j);
					pData = new TextButton(x + 2, y - 2, bsize, bsize, ids[i], mrUser + j);
					pData->fontColor = 1;
					
					// gib name
					pName = new Label(x + bsize + 6, y, gibNames[i].name);
					pName->font = qFonts[1];
					pName->fontColor = 28;
					dialog.Insert(pName);
					
					tx = x + bsize + 6, ty += pFont->height + 5;


					for (j = 0; j < len; j++, tile = plu = 0) {
						
						if (type != OBJ_WALL && gt)
						{
							if (gt->at4)
								tile = gt->at4;
							gt++;
							
						}
						else if (gf && gf->at0 >= 0 && gf->at0 < kFXMax)
						{
							FXDATA* pFx = &gFXData[gf->at0];

							if (pFx->at2)
							{ 
								getSeqPrefs(pFx->at2, &tile, &xr, &yr, &plu);
							}
							else if (pFx->at12 > 0)
							{
								tile = pFx->at12;
								plu  = pFx->at19;
							}

							gf++;
						}
							
						if (!tile)
							continue;
						
						int width, height, mx, my, tsize2 = boxsize - 4;
						tileDrawGetSize(tile, tsize2, &width, &height);
						
						// allow draw full size if tile is smaller than box
						BOOL orig = (width < tsize2 && height < tsize2);
						
						mx = tx + ((boxsize - width) >> 1);
						my = ty + ((boxsize - height) >> 1);
						
						char col = (char)(tileGetMostUsedColor(tile) ^ 16);
						dialog.Insert(new Shape(tx, ty, boxsize, boxsize, gStdColor[8])); // border
						dialog.Insert(new Shape(tx + 1, ty + 1, boxsize - 2, boxsize - 2, col)); // fill
						dialog.Insert(new Tile(mx, my, tile, (orig) ? 0 : tsize2, tsize2, (short)ClipLow(plu, 0), 0x02));
						
						tx+=boxsize+1;
						if (tx >= shapew - (bsize + 6))
							break;
						
					}
					
				}
			} 
			
			if (i >= listLen || (!len && type == OBJ_WALL)) {
				
				// button with data value (disabled)
				pData = new TextButton(x + 2, y - 2, bsize, bsize, "--", mrUser);
				pData->disabled = TRUE;
				pData->canFocus = !pData->disabled;
				pData->fontColor = 7;
				
			}

			pData->font = qFonts[1];
			dialog.Insert(pData);

			y+=(shapeh-2), ty = y;
			
		}

		i = 34, x = 4, j = ClipLow(i, 4);
		dialog.height += j + 4, y += ((j >> 2) - 6);
		BitButton2* prevPage = new BitButton2(x, y, i, i, pBitmaps[2], -1); x += i + 4; prevPage->hotKey = '1';
		BitButton2* nextPage = new BitButton2(x, y, i, i, pBitmaps[3], -2); x += i + 4; nextPage->hotKey = '2';
		
		sprintf(buffer, "Page %d of %d", page + 1, ClipLow(pages, 1));
		Label* pPage = new Label(x, (y + (i >> 1)) - (pFont->height >> 1), strupr(buffer));
		pPage->font = qFonts[1];

		sprintf(buffer, "Reset"); len = gfxGetTextLen(strupr(buffer), pFont) << 1; j = 20;
		TextButton* pNone = new TextButton(dwidth - len - 8, (y + (i >> 1)) - (j >> 1), len, j, buffer, mrUser);
		pNone->font = qFonts[1];
		
		
		dialog.Insert(prevPage);
		dialog.Insert(nextPage);
		dialog.Insert(pPage);
		dialog.Insert(pNone);
		
		ShowModal(&dialog);
		if (dialog.endState == mrCancel)
			break;
		
		switch (dialog.endState) {
			case -1: // prev page
				if (page-- <= 0) page = ClipLow(pages - 1, 0);
				continue;
			case -2: // next page
				if (++page >= pages) page = 0;
				continue;
			default:
				if (dialog.endState < mrUser) continue;
				return dialog.endState - mrUser;
		}
	}

	return -1;
}




int toolExploderSeq() {
	
	int i, spr, xspr, time = 0, sect = -1, x, y, z;
	
	static ushort amount = 3;
	static ushort minTime = 0;
	static ushort stepTime = 5;
	static ushort maxTime = 30;
	static ushort rxID = 100;
	
	if (rxID >= 100) 
		rxID = (ushort)findUnusedChannel();

	Window dialog(0, 0, 140, 160, "Exploder tool");
	
	dialog.Insert(new Label(4, 10, "AMOUNT. . . . . ."));
	EditNumber *pAmount = new EditNumber(90, 6, 40, 16, amount);
	dialog.Insert(pAmount);

	dialog.Insert(new Label(4, 30, "RX ID. . . . . . . ."));
	EditNumber *pRX = new EditNumber(90, 26, 40, 16, rxID);
	dialog.Insert(pRX);
	
	dialog.Insert(new Label(28, 58, "DELAY SETUP"));
	
	dialog.Insert(new Label(10, 72, "MIN"));
	dialog.Insert(new Label(53, 72, "STEP"));
	dialog.Insert(new Label(98, 72, "MAX"));
	
	EditNumber *pDelay1 = new EditNumber(4, 86, 40, 16, minTime);
	EditNumber *pStep = new EditNumber(48, 86, 40, 16, stepTime);
	EditNumber *pDelay3 = new EditNumber(92, 86, 40, 16, maxTime);
	dialog.Insert(pDelay1);
	dialog.Insert(pStep);
	dialog.Insert(pDelay3);

	dialog.Insert(new TextButton(4, 120, 62, 20, "&Ok", (int)mrOk));
	dialog.Insert(new TextButton(68, 120, 62, 20, "&Cancel", (int)mrCancel));

	ShowModal(&dialog);
	if (dialog.endState != mrOk)
		return mrCancel;
	
	rxID = (ushort)ClipHigh(pRX->value, 1023);
	
	updatenumsprites();
	amount   = (ushort)ClipRange(pAmount->value, 1, 1024);
	if (numsprites + amount >= kMaxSprites - 1)
		amount = (ushort)(kMaxSprites - numsprites - 1);
	
	minTime  = (ushort) ClipHigh(pDelay1->value, 4095);
	stepTime = (ushort) ClipHigh(pStep->value, 4095);
	maxTime  = (ushort) ClipRange(pDelay3->value, minTime, ClipRange(pDelay3->value, minTime, 4095));

	hit2sector(&sect, &x, &y, &z, grid);
	if (sect < 0)
		return mrCancel;
	
	for (i = 0, time = (ushort)ClipRange(time, minTime, maxTime); i < amount; i++) {

		spr = InsertSprite(sect, kStatTraps);
		spritetype* pSprite = &sprite[spr];
		pSprite->type = kTrapExploder;
		pSprite->x = x, pSprite->y = y;
		pSprite->z = z;
		
		clampSprite(pSprite);

		xspr = GetXSprite(spr);
		xsprite[xspr].waitTime = time;
		xsprite[xspr].rxID = rxID;
		
		time = (ushort)ClipRange(time + stepTime, minTime, maxTime);

	}
	
	asksave = 1;
	AutoAdjustSprites();
	scrSetMessage("%d exploder(s) created.", amount);
	BeepOk();
	
	return dialog.endState;
}

int toolXWalls2XSprites() {
	
	int i, cnt = 0, nSpr;
	for (i = 0; i < numwalls; i++) {
		
		if (wall[i].extra <= 0)
			continue;
		
		int nSect = sectorofwall(i);
		XWALL* pXWall = &xwall[wall[i].extra];
		
		switch (wall[i].type) {
			case 0: case 20: case 21:
				if (pXWall->panXVel || pXWall->panYVel) continue;
				else if (pXWall->triggerPush || pXWall->triggerVector || pXWall->triggerTouch) continue;
				else if (!pXWall->rxID || !pXWall->txID) continue;
				else if ((nSpr = InsertSprite(nSect, 0)) >= 0)
				{
					spritetype* pSpr = &sprite[nSpr];
				
					pSpr->type = wall[i].type;
					
					pSpr->x = wall[i].x;
					pSpr->y = wall[i].y;
					
					doWallCorrection(i, &pSpr->x, & pSpr->y);
					pSpr->z = getflorzofslope(nSect, pSpr->x, pSpr->y);
					
					pSpr->cstat |= kSprMoveReverse | kSprInvisible;
					pSpr->xrepeat = pSpr->yrepeat = 40;
					pSpr->picnum = 1070;
					cnt++;
					
					clampSprite(pSpr);
					
					GetXSprite(pSpr->index);
					XSPRITE* pXSpr = &xsprite[pSpr->extra];
					
					pXSpr->rxID 			= pXWall->rxID;
					pXSpr->txID 			= pXWall->txID;
					pXSpr->triggerOn 		= pXWall->triggerOn;
					pXSpr->triggerOff 		= pXWall->triggerOff;
					pXSpr->waitTime			= pXWall->waitTime;
					pXSpr->locked			= pXWall->locked;
					pXSpr->state			= pXWall->state;
					pXSpr->command			= pXWall->command;
					pXSpr->restState		= pXWall->restState;
					
					dbDeleteXWall(wall[i].extra);

				}
				break;
			default:
				continue;
			
		}
	}
	
	
	CleanUp();
	CleanUpMisc();
	return cnt;
	
}

int updViewAngle(spritetype* pSprite) {
	
	int nOctant = 0;
	if (pSprite) {
		
		int dx, dy; // get current octant
		dx = posx - pSprite->x; dy = posy - pSprite->y;
		RotateVector(&dx, &dy, -pSprite->ang + kAng45 / 2);
		nOctant = GetOctant(dx, dy);
		
	}
	
	return nOctant;
	
}

void chgSpriteZ(spritetype* pSprite, int zVal) {
	
	if (pSprite == NULL)
		return;

	pSprite->z = zVal;
	clampSprite(pSprite);
}

enum{
kRpcTileFloor	= 0x01,
kRpcTileCeil	= 0x02,
kRpcTileWall	= 0x04,
kRpcTileMasked	= 0x08,
kRpcTileSpr		= 0x10,
kRpcTileAll		= (kRpcTileFloor|kRpcTileCeil|kRpcTileWall|kRpcTileMasked|kRpcTileSpr),
};

int replacePal(int nDest) {
	
	int i, j, swal, ewal, retn = 0;
	for (i = 0; i < numsectors; i++)
	{
		sector[i].floorpal = nDest, retn++;
		sector[i].ceilingpal = nDest, retn++;
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			wall[j].pal = nDest, retn++;
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
				sprite[j].pal = nDest, retn++;
		}
	}
	
	return retn;
	
}

int replacePic(int nSrc, int nDest, char where = kRpcTileAll) {
	
	int i, j, swal, ewal, retn = 0;
	for (i = 0; i < numsectors; i++)
	{
		if ((where & kRpcTileFloor) && sector[i].floorpicnum == nSrc)
			sector[i].floorpicnum = nDest, retn++;
		
		if ((where & kRpcTileCeil) && sector[i].ceilingpicnum == nSrc)
			sector[i].ceilingpicnum = nDest, retn++;
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			if ((where & kRpcTileWall) && wall[j].picnum == nSrc)
				wall[j].picnum = nDest, retn++;
			
			if ((where & kRpcTileMasked) && wall[j].overpicnum == nSrc)
				wall[j].overpicnum = nDest, retn++;
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			if ((where & kRpcTileSpr) && sprite[j].picnum == nSrc)
				sprite[j].picnum = nDest, retn++;
		}
	}
	
	return retn;
	
}

int tileExists(BYTE* image, int wh, int hg) {
	
	int i, len = wh*hg;
	for (i = 0; i < gMaxTiles; i++)
	{
		if (wh != tilesizx[i] || hg != tilesizy[i]) continue;
		else if (tileLoadTile(i) && memcmp((void*)waloff[i], image, len) == 0)
			return i;
	}
	
	return -1;
}

enum {
kFlgMapTypeBlood	= 0x0001,
kFlgCompareTiles	= 0x0002,
kFlgImportAnim		= 0x0004,
kFlgImportView		= 0x0008,
kFlgKeepOffset		= 0x0020,
kFlgOverwrite		= 0x0040,
kFlgNoTileMap		= 0x0080,
kFlgReplace			= 0x0100,
};

NAMED_TYPE gMapArtImportErrors[] = {
	
	{ -1,	"No ART files selected" },
	{ -3,	"Not enough memory for PICS" },
	{ -4,	"Too many tiles" },
	{ -5,	"One of destination tiles is system reserved" },
	{ -6,	"Max tiles reached" },
	{ -7,	"Tile number of source tile greater than max tiles" },
	{ -8,	"Cannot overwrite already existing tile" },
	{ -999, "Unknown error"},
	
};



int mapImportArt(ARTFILE* files, int nFiles, PALETTE pal, int nStartTile, int flags) {
		
	dassert(pal != NULL);
	dassert(files != NULL);
	if (nFiles <= 0)
		return -1;
	
	//struct PLUS
	//{
		//STATUS_BIT1 used[kPluMax];
		//unsigned int count;
		
	//};
	struct PICS
	{
		unsigned int rng		: 8;
		unsigned int num		: 17;
		unsigned int newnum		: 17;
		unsigned int filenum	: 17;
		//PLUS lookup;
	};
	
	const int maxAllocUnits = 0x10000;
	int i, j, k, cnt = 0, swal, ewal, skyTiles = gSkyCount, nCurPic, nOrigPic;
	ARTFILE* file; BYTE* image; PICS* pics; PICANM pnm;
	short six, siy;
	
	BOOL keepOffset = (flags & kFlgKeepOffset);
	BOOL overwrite 	= (flags & kFlgOverwrite);
	BOOL tileMap	= (!keepOffset && !(flags & kFlgNoTileMap));
	BOOL readpnm	= ((flags & kFlgImportView) || (flags & kFlgImportAnim));

	if ((pics = (PICS*)malloc(sizeof(PICS)*maxAllocUnits)) == NULL)
		return -3;
	
	for (i = 0; i < maxAllocUnits; i++)
	{
		//memset(&pics[i].lookup, 0, sizeof(pics[i].lookup));
		pics[i].newnum	= pics[i].num = maxAllocUnits;
		pics[i].filenum = nFiles;
		pics[i].rng		= 0;
	}

	// get number of an ART file for each tile
	///////////////////////////////////////////////////
	for (i = 0; i < nFiles; i++)
	{
		for (j = files[i].start; j <= files[i].end; j++)
		{
			if (cnt >= maxAllocUnits)
			{
				free(pics);
				return -4;
			}
			
			pics[j].filenum = i;
			cnt++;
		}
	}
		
	// collect all the basic tiles for replacement
	///////////////////////////////////////////////////
	for (i = 0; i < numsectors; i++)
	{
		pics[sector[i].floorpicnum].rng		= 1;
		if (sector[i].floorstat & kSectParallax)
			pics[sector[i].floorpicnum].rng = skyTiles;
				
		pics[sector[i].ceilingpicnum].rng	= 1;
		if (sector[i].ceilingstat & kSectParallax)
			pics[sector[i].ceilingpicnum].rng = skyTiles;
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			pics[wall[j].picnum].rng		= 1;
			pics[wall[j].overpicnum].rng	= 1;
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			pics[sprite[j].picnum].rng = 1;
			if (flags & kFlgMapTypeBlood)
			{
				switch(sprite[j].type) {
					case kSwitchToggle:
					case kSwitchOneWay:
					case kDecorationTorch:
						pics[sprite[j].picnum].rng = 2;
						break;
					case kSwitchCombo:
						if (sprite[j].extra <= 0) break;
						pics[sprite[j].picnum].rng = xsprite[sprite[j].extra].data3;
						break;
				}
			}
		}
	}
	
	// lookup in palDB for correct 255s
	/////////////////////////////////////////////////////////////////////
	palFixTransparentColor(pal);
	
	// read PICANM information to add tiles that are animation or
	// view parts of the basic ones and exclude tiles that cannot be replaced
	/////////////////////////////////////////////////////////////////////
	for (i = 0; i < maxAllocUnits; i++)
	{
		if (!pics[i].rng) continue;
		else if (pics[i].filenum >= nFiles) { pics[i].rng = 0; continue; }
		else if ((flags & kFlgMapTypeBlood) || (flags & kFlgKeepOffset))
		{
			if (i == 504 || i == 2342) // ror and dynamic fire pics
			{
				pics[i].rng = 0;
				continue;
			}
		}

		if (readpnm)
		{
			file =& files[pics[i].filenum];
			readTileArt(i, file->path, NULL, NULL, NULL, NULL, &pnm);
			if ((flags & kFlgImportView) && pnm.view > 0)
			{
				if (pnm.view == kSprViewFull5)			pics[i].rng += 4;
				else if (pnm.view == kSprViewFull8)		pics[i].rng += 7;
				else if (pnm.view == kSprViewBounce)	pics[i].rng += 1;
			}
			
			if ((flags & kFlgImportAnim) && pnm.type && pnm.frames && pnm.speed)
			{
				if (pnm.type != 3) pics[i].rng += pnm.frames;
				else if (i - pnm.frames < 0) pics[i].rng = 1;
				else pics[i - pnm.frames].rng += pnm.frames; // backward anim
			}
		}
		
		if (flags & kFlgCompareTiles)
		{
			// search for tiles that already exists and set newnum to the ones that found
			for (j = i; j < i + pics[i].rng; j++)
			{
				image = NULL;
				file = &files[pics[j].filenum];
				readTileArt(j, file->path, &six, &siy, &image, NULL, NULL);
				if (image != NULL)
				{
					// compare original colors first
					if ((k = tileExists(image, six, siy)) < 0)
					{
						// compare changed colors
						remapColors((intptr_t)image, six*siy, pal);
						if ((k = tileExists(image, six, siy)) >= 0)
							pics[j].newnum = k;
					}
					else
					{
						pics[j].newnum = k;
					}

					Resource::Free(image);
				}
			}
		}
	}

	// count and resort all collected tiles
	///////////////////////////////////////////////////
	for (i = cnt = 0; i < maxAllocUnits; i++)
	{
		if (!pics[i].rng || pics[i].num < maxAllocUnits) continue;
		for (j = i; j < i + pics[i].rng; j++)
		{
			pics[cnt].filenum	= pics[j].filenum;
			pics[cnt].newnum	= pics[j].newnum;
			pics[cnt].rng		= pics[j].rng;
			pics[cnt].num		= j;
			cnt++;
		}
	}

	// give pics newnums if required and / or check for limits
	///////////////////////////////////////////////////
	for (i = 0; i < cnt;)
	{
		if (i < 0) { free(pics); return i; } // cannot continue
		else if (pics[i].newnum < maxAllocUnits) { i++; continue; } // already set
		else if (nStartTile >= gMaxTiles) i = -6;
		else if (!keepOffset)
		{
			if (tileMap)
			{
				// free range search
				for (j = 0; j < pics[i].rng; j++)
				{
					k = nStartTile + j;
					if (gSysTiles.isBusy(k)) break;
					else if (!overwrite && tilesizx[k])
						break;
				}
				
				if (j < pics[i].rng)
				{
					nStartTile += pics[i].rng;
				}
				else
				{
					k = i + pics[i].rng;
					for (j = i; j < k; j++, i++)
						pics[j].newnum = nStartTile++;
				}
			}
			else if (gSysTiles.isBusy(nStartTile)) i = -5;
			else if (!overwrite && tilesizx[nStartTile]) i = -8;
			else pics[i++].newnum = nStartTile++;
		}
		else if (pics[i].num >= gMaxTiles) i = -7;
		else if (!overwrite && tilesizx[pics[i].num]) i = -8;
		else if (gSysTiles.isBusy(pics[i].num)) i = -5;
		else pics[i].newnum = pics[i++].num;
	}
	
	// finally, start replace
	///////////////////////////////////////////////////
		
	if (flags & kFlgReplace)
	{
		// set temp tilenum so it won't cross with newnum
		for (i = 0; i < cnt; i++) replacePic(pics[i].num, 32767 - i);
	}
	
	for (i = 0; i < cnt; i++)
	{
		file = &files[pics[i].filenum]; 
		nOrigPic = pics[i].num; nCurPic = pics[i].newnum; image = NULL;
		tileFreeTile(nCurPic);
		
		readTileArt(nOrigPic, file->path, &six, &siy, &image, NULL, &pnm);
		if (image != NULL)
		{
			if (tileAllocTile(nCurPic, six, siy, 0, 0))
			{
				memcpy((void*)waloff[nCurPic], image, six*siy);
				panm[nCurPic].xcenter		= pnm.xcenter;
				panm[nCurPic].ycenter		= pnm.ycenter;
				
				if (flags & kFlgImportAnim)
				{
					panm[nCurPic].type		= pnm.type;
					panm[nCurPic].speed		= pnm.speed;
					panm[nCurPic].frames	= pnm.frames;
				}
				
				if (flags & kFlgImportView)
					panm[nCurPic].view = pnm.view;

				remapColors(waloff[nCurPic], six*siy, pal);
				switch(panm[nCurPic].view) {
					case kSprViewVoxSpin:
					case kSprViewVox:
						panm[nCurPic].view = kSprViewSingle;
						break;
				}
			}
			else
			{
				nCurPic = nOrigPic;
			}
			
			Resource::Free(image);
		}
		
		if (flags & kFlgReplace)
			replacePic(32767 - i, nCurPic);
		
		artedArtDirty(nCurPic, kDirtyAll);
		if (!keepOffset && nCurPic == nOrigPic)
			continue;
		
		viewType[nCurPic] = panm[nCurPic].view;
		if (viewType[nCurPic] == kSprViewSingle)
			tiletovox[nCurPic] = voxelIndex[nCurPic] = -1;
		
		tileShade[nCurPic] = 0;
		surfType[nCurPic]  = kSurfNone;
	}
	
	//replacePal(0);
	free(pics);
	return cnt;
}

void mapImportArtDlg(char* artPath) {
	
	PALETTE pal;
	ARTFILE* files = NULL;
	char *file = NULL, tmp[_MAX_PATH];
	int i, j = mrOk, id = -1, cnt = 0, flags, start, end, sixofs, siyofs, pnmofs, datofs, nStartTile;
	CHECKBOX_LIST importFlags[] =
	{
		{ FALSE, "This is a Blood map." },
		{ TRUE,  "Import animation frames." },
		{ TRUE,  "Import view frames." },
		{ TRUE,  "Do not import duplicates." },
		{ FALSE, "Keep original tile numbers." },
		{ FALSE, "Overwrite existing tiles." },
		{ FALSE, "Linear import." },
		//{ FALSE, "Convert colors." },
	};
	
	while( 1 )
	{
		if (j != mrCancel && artedDlgSelPal("Select palette", artPath, pal) < 0) break;
		else if ((file = dirBrowse("Select files", artPath, ".ART", kDirExpTypeOpen, kDirExpMulti)) == NULL)
		{
			j = mrOk;
			continue;
		}
		
		if (files != NULL)
			free(files);
		
		while(enumSelected(&id, tmp))
		{
			files = (ARTFILE*)realloc(files, sizeof(ARTFILE)*(cnt+1)); sprintf(files[cnt].path, tmp);
			if (readArtHead(files[cnt].path, &start, &end, &sixofs, &siyofs, &pnmofs, &datofs) < 0) continue;
			files[cnt].start = start; files[cnt].end = end;
			cnt++;
		}
		
		while((j = createCheckboxList(importFlags, LENGTH(importFlags), "Import options")) != mrCancel)
		{
			flags = 0;
			if (importFlags[0].option) flags |= kFlgMapTypeBlood;
			if (importFlags[1].option) flags |= kFlgImportAnim;
			if (importFlags[2].option) flags |= kFlgImportView;
			if (importFlags[3].option) flags |= kFlgCompareTiles;
			if (importFlags[4].option) flags |= kFlgKeepOffset;
			if (importFlags[5].option) flags |= kFlgOverwrite;
			if (importFlags[6].option) flags |= kFlgNoTileMap;
			
			nStartTile = 4608; flags |= kFlgReplace;
/* 			nStartTile = gMaxTiles - 1;
			if (flags & kFlgKeepOffset)
			{
				nStartTile = 0;
				if (!(flags & kFlgOverwrite) && Confirm("Allow overwriting?"))
					flags |= kFlgOverwrite;
			}
			else
			{
				while( 1 )
				{
					do { nStartTile--; } while(nStartTile >= 0 && !tilesizx[nStartTile]);
					nStartTile = tilePick(++nStartTile, -1, OBJ_ALL, "Pick start tile", kTilePickFlgAllowEmpty);
					if (nStartTile >= 0)
					{
						j = mrOk;
						if ((flags & kFlgNoTileMap) && !(flags & kFlgOverwrite) && tilesizx[nStartTile])
						{
							Alert("Tile %d is not blank!", nStartTile);
							continue;
						}
					}
					else
					{
						j = mrCancel;
					}
					
					break;
				}
			} */

			if (nStartTile >= 0)
			{
/*  			splashScreen();
				if (!(flags & kFlgKeepOffset) && !(flags & kFlgReplace))
				{
					if (Confirm("Replace old tiles to the ones with new number?"))
						flags |= kFlgReplace;
				} */
			
				splashScreen("This may take some time.");
				if ((i = mapImportArt(files, cnt, pal, nStartTile, flags)) < 0)
				{
					if (!Confirm("Error %d: %s. Try again?", abs(i), retnCodeCheck(i, gMapArtImportErrors))) break;
					else continue;
				}
				else
				{
					Alert("%d pictures were added or changed.", i);
					
/* 					PALETTE paint;
					Alert("%d", pluLoad("ITEST\\PALETTE.DAT", paint, 14, 0));
					

					int f = 0;
					for (i = 0; i < 256; i++)
					{
						f = countBestColor(gamepal, paint[i].r, paint[i].g, paint[i].b);
						paint[i] = gamepal[f];
					}
					artedPaint(4826, paint);
					
					artedPaint(4854, paint);
					artedPaint(4778, paint);
					artedPaint(4779, paint); */
				}
			}

			break;
		}
		
		if (j == mrCancel) continue;
		break;
	}
	
	free(files);
	
}


/* void levelImportWizard()
{
	int curPage = 0;
	int nTile = 0;
	int i, j, k,pd = 2, dw, dh, x1, x2, y1, y2;
	int hdwh, hdhg = 20, tw, th;
	RESHANDLE hIco;
	
	Window dialog(0, 0, 320, 240, "Level import wizard");
	dialog.getEdges(&x1, &y1, &x2, &y2);
	dialog.getSize(&dw, &dh);
	
	int dx = x1+pd, dy = y1+pd, banw = dw>>2, banh = dh;
	Panel* banner = new Panel(x1, y1, banw, dh, 0, 1, -1, clr2std(kColorCyan));
	dx+=banw;
	
	hIco = gGuiRes.Lookup((unsigned int)0, "TGA");
	if (hIco && (nTile = tileGetBlank()) >= 0)
	{
		tga2tile((char*)gGuiRes.Load(hIco), gGuiRes.Size(hIco), nTile);
		tileDrawGetSize(nTile, banw - (pd<<2), &tw, &th);
		Tile* logo = new Tile((banw>>1)-(tw>>1), 0, nTile, tw, th, 0, 0x02);
		banner->Insert(logo);
	}
		
	Panel* buttons = new Panel(dx, dh - 20, x2-dx, 20);
	TextButton* next = new TextButton(x2-dx-60, 0, 60, 20, "&Next", 100);
	TextButton* back = new TextButton(x2-dx-120-pd, 0, 60, 20, "&Back", 101);
	TextButton* quit = new TextButton(0, 0, 60, 20, "&Cancel", mrCancel);
	
	back->disabled = 1;
	back->fontColor = 8;
	back->canFocus = 0;
	
	buttons->Insert(next);
	buttons->Insert(back);
	buttons->Insert(quit);
	
	dialog.Insert(banner);
	dialog.Insert(buttons);
	
	while(1)
	{
		i = 0;
		if (curPage <= 0)
			i += sprintf(&buffer[i], "Welcome to the map import wizard!");
		else if (curPage == 1)
			i += sprintf(&buffer[i], "Prepare files for importing...");
		
		dx = x1 + banw + pd, dy = y1;
		Text* header = new Text(dx, y1, x2 - dx, hdhg, buffer, kTextACenter|kTextAMiddle|kTextUnderline|kTextUppercase, kColorBlue);
		dy += hdhg;
		
		i = 0;
		if (curPage <= 0)
		{
			i += sprintf(&buffer[i], "This tool helps you to import maps\n");
			i += sprintf(&buffer[i], "and game graphics from another\n");
			i += sprintf(&buffer[i], "BUILD Engine games.");
		}
		else if (curPage == 1)
		{
			i += sprintf(&buffer[i], "Select a MAP file you want to\nimport..");
		}
		
		Text* welcome = new Text(dx, dy, x2 - dx, dh>>2, buffer, kTextACenter, 0, qFonts[0]);
		dy+=dh>>2;
		
		FieldSet* pMapFileFieldset = NULL;
		if (curPage == 1)
		{
			pMapFileFieldset = new FieldSet(dx, dy, x2-dx, dh>>2, "Map file");
			EditText* pMapFileTextEd	= new EditText(4, 4, (x2-dx-8)>>1, 20, NULL);
			
			pMapFileFieldset->Insert(pMapFileTextEd);
			dialog.Insert(pMapFileFieldset);
		}
		
		dialog.Insert(header);
		dialog.Insert(welcome);

		ShowModal(&dialog);
		switch (dialog.endState) {
			case 100:
			case 101:
			dialog.Remove(header);
			dialog.Remove(welcome);
			switch (dialog.endState) {
				case 100:
					curPage++;
					continue;
				case 101:
					curPage--;
					continue;
			}
			case mrCancel:
				break;
			
		}
		
		break;
	}
	
		//i = 0, j = 0, k = LENGTH(gSuppMapVersion) - 1;
		//i += sprintf(&buffer[i], "Map versions supported:");
		//while(j < k)
		//{
			//i += sprintf(&buffer[i], " %d", gSuppMapVersion[j++]);
			//if (j < k)
				//i += sprintf(&buffer[i], ",");
		//}
		//i += sprintf(&buffer[i], " and %d.", gSuppMapVersion[j]);
		
		//Label* versions = new Label(dx, dy, buffer);
		//dialog.Insert(versions);
	
	//if (nTile >= 0)
		//tileFreeTile(nTile);
	
	return;
} */