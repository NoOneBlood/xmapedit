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

#include "common_game.h"
#include "editor.h"
#include "replace.h"
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
#include "nnexts.h"
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

NAMED_TYPE gMapErrorsCommon[] = {
	
	{ -1,	"Error opening file" },
	{ -2,	"Corrupted file" },
	{ -3, 	"Unsupported map version"},
	{ -999,	"Unknown map error"},
	
};

NAMED_TYPE gArtErrorsCommon[] = {
	
	{ -1,	"Corrupted file" },
	{ -2,	"Unsupported file format"},
	{ -10,	"localtilestart must be greater than in previous ART files"},
	{ -999, "Unknown art error"},
};

NAMED_TYPE gMapArtImportErrors[] = {
	
	{ -1,	"No ART files selected" },
	{ -3,	"Not enough memory for PICS" },
	{ -4,	"Too many tiles" },
	{ -5,	"One of destination tiles is system reserved" },
	{ -6,	"Max tiles reached" },
	{ -7,	"Tile number of source tile greater than max tiles" },
	{ -8,	"Cannot overwrite already existing tile" },
	{ -9,	"No tiles for processing" },
	{ -999, "Unknown error"},
	
};

int toolOpenWith(char* filename, char flags)
{
	RESHANDLE hFile;
	NAMED_TYPE editors[LENGTH(gExtNames)];
	char tmp[_MAX_PATH], tmp2[_MAX_PATH]; char *dsk, *dir, *fname, *ext;
	int i = 0, j = 0, k = 0, edit = -1;
	
	BOOL norff = FALSE;
	pathSplit2(filename, tmp, &dsk, &dir, &fname, &ext);
	norff = (dsk[0] || dir[0]);
	
	if (ext[0]) // extension found
	{
		if (fileExists(filename, norff ? NULL : &hFile))
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
	
	_makepath(tmp2, dsk, dir, fname, NULL);
	
	// search for files with same name but different extensions...
	for (i = 0; i < LENGTH(gExtNames); i++)
	{
		ChangeExtension(tmp2, gExtNames[i]);
		if ((k = fileExists(tmp2, norff ? NULL : &hFile)) <= 0) continue;
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

int toolLoadAsMulti(char* path, char* ext, char* title)
{
	int i = 0;
	if (dirBrowse(title, path, ext, kDirExpTypeOpen, kDirExpMulti) != NULL)
		i = countSelected();

	return i;
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

void toolSetOrigin(POINT2D* origin, int x, int y )
{
	origin->x = ClipRange(x, 0, xdim - 1);
	origin->y = ClipRange(y, 0, ydim - 1);
}

void toolDrawCenter(POINT2D* origin, int nColor, int xrng, int yrng, int skip) {
	
	int i; 
	gfxSetColor(nColor);
	for (i = origin->x - xrng; i < origin->x; i+=skip)  gfxPixel(i, origin->y);
	for (i = origin->x + xrng; i >= origin->x; i-=skip) gfxPixel(i, origin->y);
	for (i = origin->y - yrng; i < origin->y; i+=skip)  gfxPixel(origin->x, i);
	for (i = origin->y + yrng; i >= origin->y; i-=skip) gfxPixel(origin->x, i);

	
}

void toolDrawCenterHUD(POINT2D* origin, int nTile, int nVTile, int scrYofs, int nOctant, int nColor) {
	
	
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


void toolDisplayMessage(short nColor, int x, int y, QFONT* pFont)
{
	if (totalclock < gScreen.msg[0].time)
		gfxPrinTextShadow(x, y, nColor, strupr(gScreen.msg[0].text), pFont);
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


int toolChannelCleaner(char* logfile)
{
	BOOL freed[1024], tolog = FALSE;
	register int i = 2, j, c = 0, txrng[4];
	register int nChannel = 1024, hFile = -1;
	char buf[256], fmt[64];
	
	memset(freed, 0, sizeof(freed));
	sprintf(fmt, "%s", "%-6s\t#%05d\t\tRX=%-4d\t\tTX=%-4d\t\tEXTRA=%-5d\n");
	OBJECT_LIST objects[2];
	OBJECT* pDb;
		
	while(--nChannel >= 100)
	{
		c += collectObjectsByChannel(nChannel, TRUE,  &objects[0], 0x20);
		c += collectObjectsByChannel(nChannel, FALSE, &objects[1], 0x20);
	}
	
	if (c <= 0) return c;
	else if (logfile && (hFile = open(logfile, O_CREAT|O_BINARY|O_WRONLY|O_TRUNC, S_IREAD|S_IWRITE)) >= 0)
	{
		sprintf(buf, "%s Rev.%d:\n\n", gPaths.maps, gMapRev);
		write(hFile, strupr(buf), strlen(buf));
		tolog = TRUE;
	}

	while(--i >= 0)
	{
		if (tolog)
		{
			sprintf(buf, "Objects with unlinked %cX channels found:\n", (i) ? 'T' : 'R');
			write(hFile, strupr(buf), strlen(buf));
		}
		
		for(pDb = objects[i].Ptr(); pDb->type < 255; pDb++)
		{
			if (pDb->type == OBJ_SECTOR)
			{
				sectortype* pObj = &sector[pDb->index]; XSECTOR* pXObj = &xsector[pObj->extra];
				if (tolog)
					sprintf(buf, fmt, gSearchStatNames[pDb->type], pDb->index, pXObj->rxID, pXObj->txID, pObj->extra);
				
				if (!i)
				{
					freed[pXObj->rxID] = 1;
					pXObj->rxID = 0;
				}
				else
				{
					freed[pXObj->txID] = 1;
					pXObj->txID = 0;
				}
			}
			else if (pDb->type == OBJ_WALL)
			{
				walltype* pObj = &wall[pDb->index]; XWALL* pXObj = &xwall[pObj->extra];
				if (tolog)
					sprintf(buf, fmt, gSearchStatNames[pDb->type], pDb->index, pXObj->rxID, pXObj->txID, pObj->extra);
				
				if (!i)
				{
					freed[pXObj->rxID] = 1;
					pXObj->rxID = 0;
				}
				else
				{
					freed[pXObj->txID] = 1;
					pXObj->txID = 0;
				}
			}
			else if (pDb->type == OBJ_SPRITE)
			{
				spritetype* pObj = &sprite[pDb->index]; XSPRITE* pXObj = &xsprite[pObj->extra];
				if (tolog)
					sprintf(buf, fmt, gSearchStatNames[pDb->type], pDb->index, pXObj->rxID, pXObj->txID, pObj->extra);
				
				if (!i)
				{
					freed[pXObj->rxID] = 1;
					pXObj->rxID = 0;
				}
				else
				{
					if (gModernMap)
					{
						switch (pObj->type) {
							case kModernSequentialTX:
							case kModernRandomTX:
								// ranged
								if (multiTxGetRange(pDb->index, txrng))
								{
									for (j = pXObj->data1; j <= pXObj->data4; j++)
									{
										freed[j] = 1;
									}
									
									pXObj->data1 = 0;
									pXObj->data4 = 0;
								}
								// normal
								else
								{
									for (j = 0; j < 4; j++)
									{
										if (txrng[j] >= 100 && !collectObjectsByChannel(txrng[j], TRUE))
										{
											freed[txrng[j]] = 1;
											setDataValueOfObject(OBJ_SPRITE, pDb->index, j + 1, 0);
										}
									}
								}
								break;
							case kModernPlayerControl:
								if (pXObj->command == kCmdLink) continue; // the player inherits TX
								break;
							case kModernCustomDudeSpawn:
							case kMarkerDudeSpawn:
							case kModernRandom:
								if (pObj->flags & kModernTypeFlag1) continue; // spawned sprite inherits TX
								break;
						}
					}
					
					freed[pXObj->txID] = 1;
					pXObj->txID = 0;
				}
			}
			else
			{
				continue;
			}
			
			if (tolog)
				write(hFile, buf, strlen(buf));
		}
		
		
		if (tolog)
		{
			sprintf(buf, "\n");
			write(hFile, buf, strlen(buf));
		}
	}
	
	c = 0;
	while(++nChannel < 1024)
	{
		if (freed[nChannel])
			c++;
	}
	
	if (tolog)
	{
		sprintf(buf, "Channels freed in total: %d.", c);
		write(hFile, strupr(buf), strlen(buf));
		close(hFile);
	}
	
	return c;
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
		
		if (where & kRpcTileSpr)
		{
			for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			{
				if (sprite[j].picnum == nSrc)
					sprite[j].picnum = nDest, retn++;
			}
		}
	}
	
	return retn;
	
}

BOOL readData(char* file, int hOffs, BYTE* out, int len)
{
	int hFile, i;
	if ((hFile = open(file, O_BINARY|O_RDONLY, S_IWRITE|S_IREAD)) < 0) return FALSE;
	else if ((i = lseek(hFile, hOffs, SEEK_SET)) > hOffs || i < hOffs)
		return FALSE;
		
	i = read(hFile, out, len);
	close(hFile);
	return TRUE;
}

int toolMapArtGrabber(ARTFILE* files, int nFiles, PALETTE pal, int nStartTile, int skyTiles, int flags, int grabFrom) {
	
	
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
		unsigned int six		: 16;
		unsigned int siy		: 16;
		unsigned int duplicate	: 1;
		unsigned int imgofs;
		PICANM pnm;
		//PLUS lookup;
	};
	
	const int maxAllocUnits = 0x10000;
	int hFile, start, end, tiles, sixofs, siyofs, pnmofs, datofs;
	int i, j, k, cnt = 0, swal, ewal, nCurPic, nOrigPic, imgLen;
	ARTFILE* file; BYTE* image; PICS* pics; PICANM* pnm;
	char rplcFlags = 0;
	short six, siy;
	
	BOOL replace	= (flags & kFlgReplace) > 0;
	BOOL keepOffset = (flags & kFlgKeepOffset);
	BOOL overwrite 	= (flags & kFlgOverwrite);
	BOOL tileMap	= (!keepOffset && !(flags & kFlgNoTileMap));
	BOOL readpnm	= ((flags & kFlgImportView) || (flags & kFlgImportAnim));
	BOOL grabF		= (grabFrom & kGrabFloor);
	BOOL grabC		= (grabFrom & kGrabCeil);
	BOOL grabW		= (grabFrom & kGrabWall);
	BOOL grabO		= (grabFrom & kGrabOWall);
	BOOL grabS		= (grabFrom & kGrabSprite);

	if ((pics = (PICS*)malloc(sizeof(PICS)*maxAllocUnits)) == NULL)
		return -3;
	
	for (i = 0; i < maxAllocUnits; i++)
	{
		//memset(&pics[i].lookup, 0, sizeof(pics[i].lookup));
		memset(&pics[i], 0, sizeof(PICS));
		pics[i].newnum	= pics[i].num = maxAllocUnits;
		pics[i].filenum = nFiles;
	}
	
	// go through all files and fill the PICS info
	///////////////////////////////////////////////////
	for (i = 0; i < nFiles; i++)
	{
		file = &files[i];
		if ((hFile = open(file->path, O_BINARY|O_RDONLY, S_IWRITE|S_IREAD)) < 0)
			continue;
		
		readArtHead(hFile, &start, &end, &sixofs, &siyofs, &pnmofs, &datofs);
		for (j = file->start; j <= file->end; j++)
		{
			if (cnt >= maxAllocUnits)
			{
				close(hFile);
				free(pics);
				return -4;
			}
			
			lseek(hFile, sixofs, SEEK_SET);	sixofs+=read(hFile, &six, 2);
			lseek(hFile, siyofs, SEEK_SET);	siyofs+=read(hFile, &siy, 2);
			lseek(hFile, pnmofs, SEEK_SET);	pnmofs+=read(hFile, &pics[j].pnm, sizeof(PICANM));
			
			pics[j].imgofs = ((imgLen = six*siy) > 0) ? datofs : 0;
			if (pics[j].imgofs > 0)
			{
				datofs+=lseek(hFile, imgLen, SEEK_SET);	// only get image data file offset
				pics[j].six = six;
				pics[j].siy = siy;
			}
			
			pics[j].filenum = i;
			cnt++;
		}
		
		close(hFile);
	}
			
	// collect all the basic tiles for replacement
	///////////////////////////////////////////////////
	for (i = 0; i < numsectors; i++)
	{
		if (grabF)
		{
			pics[sector[i].floorpicnum].rng		= 1;
			if (sector[i].floorstat & kSectParallax)
				pics[sector[i].floorpicnum].rng = skyTiles;
		}
		
		if (grabC)
		{
			pics[sector[i].ceilingpicnum].rng	= 1;
			if (sector[i].ceilingstat & kSectParallax)
				pics[sector[i].ceilingpicnum].rng = skyTiles;
		}
		
		if (grabW || grabO)
		{
			getSectorWalls(i, &swal, &ewal);
			for (j = swal; j <= ewal; j++)
			{
				if (grabW) pics[wall[j].picnum].rng		= 1;
				if (grabO) pics[wall[j].overpicnum].rng	= 1;
			}
		}
		
		if (grabS)
		{
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
	}
		
	// lookup in palDB for correct 255s
	/////////////////////////////////////////////////////////////////////
	if (pal)
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
			pnm = &pics[i].pnm;
			if ((flags & kFlgImportView) && pnm->view > 0)
			{
				if (pnm->view == kSprViewFull5)			pics[i].rng += 4;
				else if (pnm->view == kSprViewFull8)	pics[i].rng += 7;
				else if (pnm->view == kSprViewBounce)	pics[i].rng += 1;
			}

			if ((flags & kFlgImportAnim) && pnm->type && pnm->frames)
			{	
				if (pnm->type != 3) pics[i].rng += pnm->frames;
				else if (i - pnm->frames < 0) pics[i].rng = 1;
				else pics[i - pnm->frames].rng += pnm->frames; // backward anim
			}
		}
		
		if (flags & kFlgCompareTiles)
		{
			// search for tiles that already exists and set newnum to the ones that found
			for (j = i; j < i + pics[i].rng; j++)
			{ 
				if (!pics[j].imgofs)
					continue;
				
				file = &files[pics[j].filenum];
				six = pics[j].six; siy = pics[j].siy;
				image = NULL;
				
				imgLen = six*siy;
				if ((image = (BYTE*)malloc(imgLen)) == NULL) continue;
				else if (readData(file->path, pics[j].imgofs, image, imgLen))
				{
					// compare original colors first
					if ((k = tileExists(image, six, siy)) < 0)
					{
						if (pal)
						{
							// compare changed colors
							remapColors((intptr_t)image, imgLen, pal);
							if ((k = tileExists(image, six, siy)) >= 0)
							{
								pics[j].newnum		= k;
								pics[j].duplicate	= 1;
							}
						}
					}
					else
					{
						pics[j].newnum 		= k;
						pics[j].duplicate	= 1;
					}
				}
				
				free(image);
			}
		}
	}

	// count and resort all collected tiles
	///////////////////////////////////////////////////
	for (i = cnt = 0; i < maxAllocUnits; i++)
	{
		if (!pics[i].rng) continue;
		for (j = i; j < i + pics[i].rng; j++)
		{
			pics[cnt] = pics[j];
			pics[cnt].num = j;
			cnt++;
		}
	}
	
	
	// change all blank tilenums to the nStartTile
	///////////////////////////////////////////////////
	if (!keepOffset)
	{
		for (i = 0; i < cnt; i++)
		{
			if (pics[i].imgofs)
				continue;
			
			j = nStartTile++;
			while(i < cnt)
			{
				if (!pics[i].imgofs)
				{
					pics[i].newnum		= j;
					pics[i].duplicate	= 1;
				}
				
				i++;
			}
			
			artedEraseTileFull(j);
			break;
			
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
	if (replace)
	{
		if (grabF)	rplcFlags |= kRpcTileFloor;
		if (grabC)	rplcFlags |= kRpcTileCeil;
		if (grabW)	rplcFlags |= kRpcTileWall;
		if (grabO)	rplcFlags |= kRpcTileMasked;
		if (grabS)	rplcFlags |= kRpcTileSpr;
		
		// set temp tilenum so it won't cross with newnum
		for (i = 0; i < cnt; i++)
			replacePic(pics[i].num, 32767 - i, rplcFlags);
	}
	
	for (i = 0; i < cnt; i++)
	{
		file = &files[pics[i].filenum]; 
		nOrigPic = pics[i].num; nCurPic = pics[i].newnum; image = NULL;
		six = pics[i].six; siy = pics[i].siy; pnm = &pics[i].pnm;
		imgLen = six*siy;
		
		if (!pics[i].duplicate)
		{
			artedEraseTileFull(nCurPic);
			if (pics[i].imgofs && (image = tileAllocTile(nCurPic, six, siy, 0, 0)) != NULL)
			{
				if (readData(file->path, pics[i].imgofs, image, imgLen))
				{
					panm[nCurPic].xcenter		= pnm->xcenter;
					panm[nCurPic].ycenter		= pnm->ycenter;
					
					if (flags & kFlgImportAnim)
					{
						panm[nCurPic].type		= pnm->type;
						panm[nCurPic].speed		= pnm->speed;
						panm[nCurPic].frames	= pnm->frames;
					}
					
					if (flags & kFlgImportView)
						panm[nCurPic].view = pnm->view;
					
					if (pal)
						remapColors(waloff[nCurPic], imgLen, pal);
					
					switch(panm[nCurPic].view) {
						case kSprViewVoxSpin:
						case kSprViewVox:
							panm[nCurPic].view = kSprViewSingle;
							break;
					}
				}
			}
			
			artedArtDirty(nCurPic, kDirtyAll);
		}
			
		if (replace)
			replacePic(32767 - i, nCurPic, rplcFlags);
		
		viewType[nCurPic] = panm[nCurPic].view;
		if (viewType[nCurPic] == kSprViewSingle)
			tiletovox[nCurPic] = voxelIndex[nCurPic] = -1;
		
		if (!keepOffset && nCurPic == nOrigPic)
			continue;

		surfType[nCurPic]  = kSurfNone;
		tileShade[nCurPic] = 0;
	}
	
	free(pics);
	return (cnt > 0) ? cnt : -9;
}

int IMPORT_WIZARD::ShowDialog(IMPORT_WIZARD_MAP_ARG* pMapArg)
{
	#define kBrowseColor		gStdColor[21]
	#define kBannerPanelColor	gStdColor[kColorBlue]
	#define kFieldBorderColor	23
	#define kFieldTitleColor	kColorDarkGray
	#define kHeaderFlags		kTextACenter|kTextAMiddle|kTextUnderline|kTextUppercase
	#define kHeaderTextColor	kColorBlue
	
	Init();
	
	char* guiTxt[] =
	{
		"<not selected>",
		"...",
		"&Next",
		"&Import",
	};
	
	BOOL DLGUPD = TRUE;
	IMPORT_WIZARD_PREFS* pPrefs =& gImportPrefs;
	IOBuffer* pMapIo = NULL; BYTE* pScreen = NULL;
	QFONT* pTextFont = qFonts[2]; Panel* pPage = NULL;
	const int winW = 320, winH = 240, hdhg = 20, btw = 60, bth = 20, pd = 2;
	int start, end, sixofs, siyofs, pnmofs, datofs, pgx, pgy, pgw, pgh, tw, th;
	int i, j, k, dw, dh, x1, x2, y1, y2, dx, dy, banw, artFlags, retn = 0;
	char pthmap[_MAX_PATH], pthpal[_MAX_PATH], pthart[_MAX_PATH];
	char tmp[_MAX_PATH], *errMsg = NULL;
	
	sprintf(pthmap, gPaths.maps);
	sprintf(pthart, gPaths.maps);
	sprintf(pthpal, gPaths.maps);

	Window dialog((xdim - winW)>>1, (ydim - winH)>>1, winW, winH, "Import wizard");
	dialog.getEdges(&x1, &y1, &x2, &y2);
	dialog.getSize(&dw, &dh);
	
	dx = x1+pd, dy = y1+pd, banw = dw>>2;
	Panel* pBanner = new Panel(x1, y1, banw, dh, 0, 1, -1, kBannerPanelColor);
	dx+=banw;
	
	pgx = dx+4, pgy = y1, pgw = x2-dx-8, pgh = y2-30;
	
	tileDrawGetSize(gSysTiles.icoXmp, perc2val(banw, 75), &tw, &th);
	Tile* tLogo = new Tile((banw>>1)-(tw>>1), pd<<1, gSysTiles.icoXmp, tw, th, 0, 0x02);
	
	Panel* pButtons 		= new Panel(dx, dh-bth, x2-dx, bth);
	TextButton* bNext 		= new TextButton(pButtons->width-btw, 0, btw, bth, guiTxt[2], 100);
	TextButton* bBack 		= new TextButton(pButtons->width-pd-(btw<<1), 0, btw, bth, "&Back", 101);
	TextButton* bQuit 		= new TextButton(0, 0, btw, bth, "&Quit", mrCancel);
		
	Checkbox* cImpArt		= new Checkbox(0, 0,  pPrefs->mapImpotArt,			"Import graphics.");	
	Checkbox* cErsi			= new Checkbox(4, 0,  pPrefs->mapErsInfo, 			"Erase object info.");
	Checkbox* cErsp			= new Checkbox(4, 13, pPrefs->mapErsPal,			"Reset palookups.");
	Checkbox* cErss			= new Checkbox(4, 26, pPrefs->mapErsSpr,			"Delete sprites.");
	
	Checkbox* cKeepOfs		= new Checkbox(4, 26,  pPrefs->artKeepOffset,		"Keep original tile numbers.", 131);
	Checkbox* cImpAnim		= new Checkbox(4, 8,   pPrefs->artImportAnim,		"Import animation frames.");
	Checkbox* cImpView		= new Checkbox(4, 21,  pPrefs->artImportView,		"Import view frames (blood).");
	Checkbox* cTileMap		= new Checkbox(4, 34,  pPrefs->artTileMap,			"Use \"Tile Roadmap\" method.");
	Checkbox* cReplaceOld	= new Checkbox(4, 47,  pPrefs->artChgTilenums,		"Change object tilenums to new.");
	Checkbox* cNoDuplicate	= new Checkbox(4, 60,  pPrefs->artNoDuplicates,		"Do not import duplicates.");
		
	Label* lMapFile 		= new Label(4, 0, guiTxt[0], kColorRed);
	Label* lArtFile 		= new Label(4, 0, guiTxt[0], kColorRed);
	Label* lPalFile			= new Label(4, 0, guiTxt[0], kColorRed);
	Label* lArea			= new Label(4, 0, guiTxt[0], kColorRed);
	
	TextButton* bMapBrow	= new TextButton(4, 4, 30, 20, guiTxt[1], 102);
	TextButton* bArtBrow	= new TextButton(4, 4, 30, 20, guiTxt[1], 110);
	TextButton* bPalBrow	= new TextButton(4, 4, 30, 20, guiTxt[1], 120);
	TextButton* bAreaBrow	= new TextButton(4, 4, 30, 20, guiTxt[1], 130);
	
	sprintf(tmp, "ver %d.%d", kWizardVerMajor, kWizardVerMinor);
	i = gfxGetTextLen(tmp, pFont); j = pFont->height<<1;
	Text* tVer = new Text(0, 0, i+(pd<<2), j, tmp, kTextACenter|kTextAMiddle|kTextUppercase, kColorRed, pFont, kColorBlack);
	tVer->left = abs((pBanner->width>>1)-(tVer->width>>1));
	tVer->top  = pBanner->height-j-(pd<<2);
	
	//--------------------------------------------------------
	Panel* pageMap = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Welcome to the import wizard tool!";
		char* tWelcDesc = "This tool helps you to import maps\n"
						  "and/or graphics from another\n"
						  "BUILD Engine game.";
		// ---------------------------------------------------
		Text* head = new Text(0, dy, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height;
		// ---------------------------------------------------
		Text* tWelcText = new Text(0, dy, pgw, 0, tWelcDesc, kTextACenter, kColorBlack, pTextFont);
		tWelcText->height = pFont->height*tWelcText->lines;
		dy+=tWelcText->height+10;
		// ---------------------------------------------------
		i = 0, j = 0, k = LENGTH(gSuppMapVersions) - 1;
		i += sprintf(&tmp[i], "Map file versions supported:\n");
		while(j < k)
		{
			i += sprintf(&tmp[i], " %d", gSuppMapVersions[j++]);
			if (j < k)
				i += sprintf(&tmp[i], ",");
		}
		i += sprintf(&tmp[i], " and %d.", gSuppMapVersions[j]);
		Text* tVerText = new Text(0, dy, pgw, 0, tmp, kTextACenter|kTextAMiddle, kColorRed, pTextFont);
		tVerText->height = pFont->height*tVerText->lines;
		dy+=tVerText->height+10;
		// ---------------------------------------------------
		
		FieldSet* fMapFile  = new FieldSet(0, dy, pgw, 78, "SELECT THE MAP FILE", kFieldTitleColor, kFieldBorderColor);
		Panel* pMapFile		= new Panel(4, 6, fMapFile->width-8, 26, 0, 0, 0, kBrowseColor);
		lMapFile->top 		= abs((pMapFile->height>>1)-(pFont->height>>1));
		bMapBrow->left		= pMapFile->width-bMapBrow->width-4;
		bMapBrow->top		= abs((pMapFile->height>>1)-(20>>1));

		pMapFile->Insert(lMapFile);		
		pMapFile->Insert(bMapBrow);
		fMapFile->Insert(pMapFile);
		
		dy = pMapFile->height+10;
		cErsi->top+=dy;	fMapFile->Insert(cErsi);
		cErsp->top+=dy;	fMapFile->Insert(cErsp);
		cErss->top+=dy;	fMapFile->Insert(cErss);
		
		cImpArt->left = pageMap->width-cImpArt->width-4;
		cImpArt->top  = pageMap->height-cImpArt->height-4;
		
		pageMap->Insert(head);
		pageMap->Insert(tWelcText);
		pageMap->Insert(fMapFile);
		pageMap->Insert(tVerText);
		pageMap->Insert(cImpArt);

	}
	//--------------------------------------------------------
	Panel* pageArt = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Prepare game graphics for import";
		char* tArtDesc = "Select an ART files that contains\nthe graphics which used\nin this map.";
		char* tPalDesc = "Select a correct palette that\nmatching the content of the\nselected art files.";
		
		Text* head  = new Text(0, 0, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height+8;
		//------------------------------------------------------------------
		FieldSet* fArtFile		= new FieldSet(0, dy, pgw, 68, "ART FILES", kFieldTitleColor, kFieldBorderColor);
		Panel* pArtFile			= new Panel(4, 6, fArtFile->width-8, 26, 0, 0, 0, kBrowseColor);
		lArtFile->top			= abs((pArtFile->height>>1)-(pFont->height>>1));
		bArtBrow->left			= pArtFile->width-bArtBrow->width-4;
		bArtBrow->top			= abs((pArtFile->height>>1)-(20>>1));
		Text* tArtText 			= new Text(4, 36, fArtFile->width-8, 0, tArtDesc, kTextACenter, kColorBlack, pTextFont);
		tArtText->height 		= pFont->height*tArtText->lines;
		dy+=fArtFile->height+16;
		//------------------------------------------------------------------
		FieldSet* fPalFile		= new FieldSet(0, dy, pgw, 68, "PALETTE", kFieldTitleColor, kFieldBorderColor);
		Panel* pPalFile			= new Panel(4, 6, fPalFile->width-8, 26, 0, 0, 0, kBrowseColor);
		bPalBrow->left			= pPalFile->width-bPalBrow->width-4;
		bPalBrow->top			= abs((pPalFile->height>>1)-(20>>1));
		lPalFile->top			= abs((pPalFile->height>>1)-(pFont->height>>1));
		Text* tPalText 			= new Text(4, 36, fPalFile->width-8, 0, tPalDesc, kTextACenter, kColorBlack, pTextFont);
		tPalText->height 		= pFont->height*tPalText->lines;
		
		pArtFile->Insert(lArtFile);		fArtFile->Insert(pArtFile);
		pArtFile->Insert(bArtBrow);		fArtFile->Insert(tArtText);

		pPalFile->Insert(lPalFile);		fPalFile->Insert(pPalFile);
		pPalFile->Insert(bPalBrow);		fPalFile->Insert(tPalText);

		pageArt->Insert(head);
		pageArt->Insert(fArtFile);
		pageArt->Insert(fPalFile);
	}
	//--------------------------------------------------------
	Panel* pageArtOpts = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Additional graphics import options";
		char* tAreaDesc = "Select start tile number";
		Text* head  = new Text(0, 0, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height+8;
		
		//------------------------------------------------------------------
		FieldSet* fArea			= new FieldSet(0, dy, pgw, 54, "DEFINE STARTING TILE", kFieldTitleColor, kFieldBorderColor);
		Panel* pArea			= new Panel(4, 6, fArea->width-8, 26, 0, 0, 0, kBrowseColor);
		bAreaBrow->left			= pArea->width-bAreaBrow->width-4;
		bAreaBrow->top			= abs((pArea->height>>1)-(20>>1));
		lArea->top				= abs((pArea->height>>1)-(pFont->height>>1));
		dy+=fArea->height+16;
		//------------------------------------------------------------------
		FieldSet* fAdva			= new FieldSet(0, dy, pgw, 77, "ADVANCED SETTINGS", kFieldTitleColor, kFieldBorderColor);
		
		fAdva->Insert(cImpAnim);
		fAdva->Insert(cImpView);
		fAdva->Insert(cTileMap);
		//fAdva->Insert(cOverExist);
		fAdva->Insert(cReplaceOld);
		fAdva->Insert(cNoDuplicate);
		
		cKeepOfs->top = pArea->height+10;
		
		pArea->Insert(lArea);		fArea->Insert(pArea);
		pArea->Insert(bAreaBrow);	fArea->Insert(cKeepOfs);
	
		pageArtOpts->Insert(head);
		pageArtOpts->Insert(fArea);
		pageArtOpts->Insert(fAdva);
		
	}
	//--------------------------------------------------------
	Panel* pagePrep = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Everything is ready for import!";
		char* desc1 = "Click \"Import\" button to start\n"
					  "or \"Back\" to edit the settings.\n\n"
					  "Please, do not interrupt the import\n"
					  "process even if it seems like\n"
					  "is not working.";
					  
		Text* head  	= new Text(0, 0, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height;
		
		Text* tDesc1	= new Text(0, dy, pgw, 0, desc1, kTextACenter, kColorBlack, pTextFont);
		tDesc1->height	= pTextFont->height*tDesc1->lines;
		tDesc1->top 	= ((pagePrep->height-head->height)>>1)-(tDesc1->height>>1);
		
		pagePrep->Insert(head);
		pagePrep->Insert(tDesc1);
	}
	//--------------------------------------------------------
	Panel* pageError = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Something goes the wrong way...";
		char* desc1 = "An error occured while importing\n"
					  "the selected items.\n\n"
					  "Click \"Back\" button to edit settings\n"
					  "or \"Quit\" to quit.";
		
		Text* head  = new Text(0, 0, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height;
		
		Text* tDesc1	= new Text(0, dy, pgw, 0, desc1, kTextACenter, kColorBlack, pTextFont);
		tDesc1->height	= pTextFont->height*tDesc1->lines;
		tDesc1->top 	= ((pagePrep->height-head->height)>>1)-(tDesc1->height>>1);
		
		pageError->Insert(head);
		pageError->Insert(tDesc1);
	}
	//--------------------------------------------------------
	Panel* pageDone = new Panel(pgx, pgy, pgw, pgh);
	{
		int dy = 0;
		char* headText = "Finishing the import process!";
		char* desc1    = "Selected items were successfully\nimported!\n\n\n"
					     "Click \"Back\" button to import\n"
					     "something else or \"Quit\" to\n"
					     "quit.";
					  
		Text* head  = new Text(0, 0, pgw, hdhg, headText, kHeaderFlags, kHeaderTextColor);
		dy+=head->top+head->height;
		
		Text* tDesc1	= new Text(0, dy, pgw, 0, desc1, kTextACenter, kColorBlack, pTextFont);
		tDesc1->height	= pTextFont->height*tDesc1->lines;
		tDesc1->top 	= ((pagePrep->height-head->height)>>1)-(tDesc1->height>>1);
		
		pageDone->Insert(head);
		pageDone->Insert(tDesc1);
	}

	pBanner->Insert(tLogo);
	pBanner->Insert(tVer);
	
	pButtons->Insert(bNext);
	pButtons->Insert(bBack);
	pButtons->Insert(bQuit);
	
	pPage = pageMap;
	dialog.Insert(pBanner);
	dialog.Insert(pPage);
	dialog.Insert(pButtons);
	
	DlgDisableButton(bBack);
	
	if (pMapArg)
	{
		sprintf(pthmap, pMapArg->filepath);
		mapVersion 		= pMapArg->version;
		bloodMap 		= pMapArg->blood;
		pMapIo 			= pMapArg->pIo;
		selmap 			= 1;
		
		lMapFile->fontColor = kColorDarkGray;
		sprintf(pthmap, pMapArg->filepath);
		DlgDisableButton(bMapBrow);
		dialog.SetFocusOn(bNext);
	}
	else
	{
		DlgDisableButton(bNext);
		dialog.SetFocusOn(bMapBrow);
	}
	
	while( 1 )
	{
		if (DLGUPD)
		{
			if (cKeepOfs->checked)
			{
				lArea->fontColor = kColorDarkGray;
				DlgDisableButton(bAreaBrow);
				DlgDisableCheckbox(cTileMap, 0);
				DlgDisableCheckbox(cReplaceOld, 1);
			}
			else
			{
				lArea->fontColor = kColorRed;
				DlgEnableButton(bAreaBrow);
				DlgEnableCheckbox(cTileMap);
				DlgEnableCheckbox(cReplaceOld);
			}
			
			if (selmap)
			{
				getFilename(pthmap, tmp, TRUE);
				sprintf(lMapFile->string, "%s (v%d, %s)", tmp, mapVersion, (bloodMap) ? "Blood" : "Build");
				strupr(lMapFile->string);
				
				if (pPage == pageMap)
					DlgEnableButton(bNext);
				
				if (!bloodMap)
					DlgDisableCheckbox(cImpView, 0);
			}
			
			sprintf(lArea->string, "Tile #%d", nStartTile);
			strupr(lArea->string);
			DLGUPD = FALSE;
		}
		
		ShowModal(&dialog, kModalNoCenter|kModalNoFocus);
		dialog.ClearFocus();
				
		switch (dialog.endState) {
			case 100:	// prev page
			//------------------------------------------------------------------
				dialog.Remove(pPage);
				
				if (pPage == pageMap)			pPage = (cImpArt->checked) ? pageArt : pagePrep;
				else if (pPage == pageArt)		pPage = pageArtOpts;
				else if (pPage == pageArtOpts)	pPage = pagePrep;
				
				dialog.Insert(pPage);
				
				if (pPage != pageMap) DlgEnableButton(bBack);
				if (pPage == pageArt && (!selpal || !nArtFiles)) DlgDisableButton(bNext);
				else if (pPage == pagePrep)
				{
					bNext->text		= guiTxt[3];
					bNext->result	= 103;
				}
				continue;
			case 101:	// next page
			//------------------------------------------------------------------
				dialog.Remove(pPage);
				if (pPage == pagePrep || pPage == pageError || pPage == pageDone)
				{
					pPage = (cImpArt->checked) ? pageArtOpts : pageMap;
					bNext->text = guiTxt[2];
					bNext->result = 100;
					DlgEnableButton(bNext);
				}
				else if (pPage == pageArtOpts)
				{
					pPage = pageArt;
				}
				else if (pPage == pageArt)
				{
					pPage = pageMap;
					DlgDisableButton(bBack);
				}
				
				if (pPage == pageMap && selmap) DlgEnableButton(bNext);
				dialog.Insert(pPage);
				continue;
			case 102:	// select map
			//------------------------------------------------------------------
				while ( 1 )
				{
					errMsg = NULL;
					if (pMapData)
						free(pMapData), pMapData = NULL;
					
					if (!toolLoadAs(gPaths.maps, ".map", "Select map")) break;
					else if ((i = fileLoadHelper(gPaths.maps, &pMapData)) >= 0)
					{
						pMapIo = new IOBuffer(i, pMapData);
						i = MapCheckFile(pMapIo, gSuppMapVersions, LENGTH(gSuppMapVersions), &bloodMap);
						
					}
					
					if (i < 0)
						errMsg = retnCodeCheck(i, gMapErrorsCommon);
					
					getFilename(gPaths.maps, tmp, TRUE);
					if (!errMsg)
					{
						sprintf(pthmap, gPaths.maps);
						mapVersion = i;
						DLGUPD = TRUE;
						selmap = 1;
						break;
					}
					else
					{
						Alert("Error #%d in %s: %s", abs(i), strupr(tmp), errMsg);
					}
				}
				continue;
			case 103:	// start import
				i = -1;
				
				// import map
				if (selmap)
				{
					boardReset();
					
					if (!bloodMap)
					{
						i = dbLoadBuildMap(pMapIo);
					}
					else
					{
						i = dbLoadMap(pMapIo, pthmap);
					}
								
					if (i >= 0)
					{
						if (cErss->checked) MapEraseSprites();
						if (cErsi->checked) MapEraseInfo();
						if (cErsp->checked) MapErasePalookups();
						gMapLoaded = TRUE;
						sprintf(gPaths.maps, pthmap);
						updatesector(posx, posy, &cursectnum);
						updatenumsprites();
						clampCamera();
						retn |= 0x01;
					}
				}
				
				// import art
				if (cImpArt->checked)
				{
					artFlags = kFlgOverwrite;
					if (cReplaceOld->checked)	artFlags |= kFlgReplace;
					if (bloodMap)				artFlags |= kFlgMapTypeBlood;
					if (cKeepOfs->checked)		artFlags |= kFlgKeepOffset;
					if (cImpAnim->checked)		artFlags |= kFlgImportAnim;
					if (cImpView->checked)		artFlags |= kFlgImportView;
					if (cNoDuplicate->checked)	artFlags |= kFlgCompareTiles;
					if (!cTileMap->checked)		artFlags |= kFlgNoTileMap;
					
					i = toolMapArtGrabber(pArtFiles, nArtFiles, pal, nStartTile, gSkyCount, artFlags);
					if (i < 0)
						Alert("ART Import Error #%d: %s.", abs(i), retnCodeCheck(i, gMapArtImportErrors));
					else
						retn |= 0x02;
				}
				
				dialog.Remove(pPage);
				pPage = pageError;
				if (selmap && (retn & 0x01))
				{
					MapClipPicnums();
					CleanUp();
					
					processDrawRooms();
					if (!cImpArt->checked || (retn & 0x02))
						pPage = pageDone;
					
				}
				DlgDisableButton(bNext);
				dialog.Insert(pPage);
				continue;
			case 110:	// select art
			case 120:	// select palette
			//------------------------------------------------------------------
			switch (dialog.endState) {
				case 110:
				//------------------------------------------------------------------
					if (toolLoadAsMulti(gPaths.maps, ".art", "Select files") > 0)
					{
						if (pArtFiles)
							free(pArtFiles), pArtFiles = NULL;
						
						k = -1;
						nArtFiles = 0, i = -1;
						while(enumSelected(&i, tmp))
						{
							errMsg = NULL;
							pArtFiles = (ARTFILE*)realloc(pArtFiles, sizeof(ARTFILE)*(nArtFiles+1));
							
							sprintf(pArtFiles[nArtFiles].path, tmp);
							j = readArtHead(pArtFiles[nArtFiles].path, &start, &end, &sixofs, &siyofs, &pnmofs, &datofs);						
							if (j < 0)
								errMsg = retnCodeCheck(j, gArtErrorsCommon);
							
							// additional check for localtilestart because of 7paladins...
							if (!errMsg && start == k)
								j = -10, errMsg = retnCodeCheck(j, gArtErrorsCommon);
							
							k = start;
							char tmp2[_MAX_PATH];
							getFilename(tmp, tmp2, TRUE);
							if (!errMsg)
							{
								pArtFiles[nArtFiles].start = start; pArtFiles[nArtFiles].end = end;
								nArtFiles++;
							}
							else if (!Confirm("Error #%d in %s: %s. Skip it?", abs(j), tmp2, errMsg))
							{
								nArtFiles = 0;
								break;
							}
						}
						
						if (nArtFiles)
						{
							sprintf(lArtFile->string, "%d files selected", nArtFiles);
							sprintf(pthart, gPaths.maps);
							strupr(lArtFile->string);
						}
					}
					break;
				case 120:
				//------------------------------------------------------------------
					if (artedDlgSelPal("Load palette", gPaths.maps, pal) < 0) continue;
					getFilename(gPaths.maps, tmp, TRUE);
					sprintf(lPalFile->string, strupr(tmp));
					sprintf(pthpal, gPaths.maps);
					selpal = 1;
					break;
			}
			if (nArtFiles && selpal) DlgEnableButton(bNext); else DlgDisableButton(bNext);
			continue;
			//------------------------------------------------------------------
			case 130:	// select starting tile
				j = bytesperline*ydim;
				if ((pScreen = (BYTE*)malloc(j)) != NULL)
					memcpy(pScreen, (void*)frameplace, j);
				
				while( 1 )
				{
					if (pScreen) memcpy((void*)frameplace, pScreen, j);
					if ((i = tilePick(nStartTile, -1, OBJ_ALL, "Pick start tile (can be empty)", kTilePickFlgAllowEmpty)) < 0)
						break;
					
					nStartTile = i;
					break;
				}
				
				if (pScreen)
				{
					memcpy((void*)frameplace, pScreen, j);
					free(pScreen), pScreen = NULL;
				}
				// no break
			case 131:
				DLGUPD = TRUE;
				continue;
			//------------------------------------------------------------------
			case mrCancel:
				if (!retn) retn = -1;
				break;
			//------------------------------------------------------------------
			default:
				continue;
		}
		
		break;
	}
	
	Free();
	
	// save ckeckbox states
	pPrefs->mapErsSpr 		= cErss->checked;
	pPrefs->mapErsPal 		= cErsp->checked;
	pPrefs->mapErsInfo 		= cErsi->checked;
	pPrefs->mapImpotArt		= cImpArt->checked;
	
	pPrefs->artKeepOffset	= cKeepOfs->checked;
	pPrefs->artImportAnim	= cImpAnim->checked;
	pPrefs->artImportView	= cImpView->checked;
	pPrefs->artTileMap		= cTileMap->checked;
	pPrefs->artChgTilenums	= cReplaceOld->checked;
	pPrefs->artNoDuplicates	= cNoDuplicate->checked;
	
	return retn;
	
}

void IMPORT_WIZARD::DlgDisableButton(TextButton* pButton)
{
	pButton->disabled  = 1;
	pButton->canFocus  = 0;
	pButton->fontColor = kColorDarkGray;
}

void IMPORT_WIZARD::DlgEnableButton(TextButton* pButton, char fontColor)
{
	pButton->disabled  = 0;
	pButton->canFocus  = 1;
	pButton->fontColor = fontColor;
}

void IMPORT_WIZARD::DlgDisableCheckbox(Checkbox* pButton, int checked)
{
	pButton->disabled  = 1;
	pButton->canFocus  = 0;
	
	if (checked == 0)
		pButton->checked = 0;
	else if (checked == 1)
		pButton->checked = 1;
}

void IMPORT_WIZARD::DlgEnableCheckbox(Checkbox* pButton, int checked, char fontColor)
{
	pButton->disabled  = 0;
	pButton->canFocus  = 1;
	
	if (checked == 0)
		pButton->checked = 0;
	else if (checked == 1)
		pButton->checked = 1;
}

void IMPORT_WIZARD::MapClipPicnums()
{
	int i, j, swal, ewal, nPicMax = gMaxTiles-1;
	for (i = 0; i < numsectors; i++)
	{
		sectortype* pSect = &sector[i];
		pSect->floorpicnum = ClipRange(pSect->floorpicnum, 0, nPicMax);
		pSect->ceilingpicnum = ClipRange(pSect->ceilingpicnum, 0, nPicMax);
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			walltype* pWall = &wall[j];
			pWall->picnum = ClipRange(pWall->picnum, 0, nPicMax);
			pWall->overpicnum = ClipRange(pWall->overpicnum, 0, nPicMax);
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			spritetype* pSpr = &sprite[j];
			pSpr->picnum = ClipRange(pSpr->picnum, 0, nPicMax);
		}
	}
}

void IMPORT_WIZARD::ArtSetStartTile()
{
	nStartTile = gMaxTiles - 1;
	do { nStartTile--; } while(nStartTile >= 0 && !tilesizx[nStartTile] && !isSysTile(nStartTile));
	nStartTile++;
}

void IMPORT_WIZARD::MapEraseSprites()
{
	int i, j;
	for (i = 0; i < numsectors; i++)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			if (bloodMap && sprite[j].statnum == kStatMarker)
				continue;
			
			DeleteSprite(j);
		}
	}
}

void IMPORT_WIZARD::MapErasePalookups(char nPalookup)
{
	int i, j, swal, ewal;
	for (i = 0; i < numsectors; i++)
	{
		sector[i].floorpal	 = nPalookup;
		sector[i].ceilingpal = nPalookup;
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
			wall[j].pal = nPalookup;
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			sprite[j].pal = nPalookup;
	}		
}

void IMPORT_WIZARD::MapEraseInfo()
{
	short cstat;
	int i, j, swal, ewal;
	for (i = 0; i < numsectors; i++)
	{
		sectortype* pSect = &sector[i];
		
		pSect->alignto = 0;
		pSect->hitag = pSect->lotag = 0;	
		if (bloodMap)
		{
			if (pSect->extra > 0)
				dbDeleteXSector(pSect->extra);
		}
		else
		{
			// cleanup cstat			
			cstat = pSect->floorstat;	pSect->floorstat = 0;
			if (cstat & kSectParallax)	pSect->floorstat |= kSectParallax;
			if (cstat & kSectSloped)	pSect->floorstat |= kSectSloped;
			if (cstat & kSectExpand)	pSect->floorstat |= kSectExpand;
			if (cstat & kSectRelAlign)	pSect->floorstat |= kSectRelAlign;
			switch ((j = (cstat & kSectFlipMask)))
			{
				case 0x04:	case 0x10:
				case 0x14:	case 0x20:
				case 0x24:	case 0x30:
				case 0x34:	pSect->floorstat |= (BYTE)j;	break;
			}
			
			cstat = pSect->ceilingstat;	pSect->ceilingstat = 0;
			if (cstat & kSectParallax)	pSect->ceilingstat |= kSectParallax;
			if (cstat & kSectSloped)	pSect->ceilingstat |= kSectSloped;
			if (cstat & kSectExpand)	pSect->ceilingstat |= kSectExpand;
			if (cstat & kSectRelAlign)	pSect->ceilingstat |= kSectRelAlign;
			switch ((j = (cstat & kSectFlipMask)))
			{
				case 0x04:	case 0x10:
				case 0x14:	case 0x20:
				case 0x24:	case 0x30:
				case 0x34:	pSect->ceilingstat |= (BYTE)j;	break;
			}
		}
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			walltype* pWall = &wall[j];
			pWall->hitag = pWall->lotag = 0;
			if (bloodMap)
			{
				if (pWall->extra > 0)
					dbDeleteXWall(pWall->extra);
			}
			else
			{
				// cleanup cstat
				cstat = pWall->cstat;		pWall->cstat = 0;
				if (cstat & kWallBlock)		pWall->cstat |= kWallBlock;
				if (cstat & kWallSwap)		pWall->cstat |= kWallSwap;
				if (cstat & kWallOrgBottom)	pWall->cstat |= kWallOrgBottom;
				if (cstat & kWallFlipX)		pWall->cstat |= kWallFlipX;
				if (cstat & kWallFlipY)		pWall->cstat |= kWallFlipY;
				if (cstat & kWallMasked)	pWall->cstat |= kWallMasked;
				if (cstat & kWallOneWay)	pWall->cstat |= kWallOneWay;
				if (cstat & kWallHitscan)	pWall->cstat |= kWallHitscan;
				if (cstat & kWallTransluc)	pWall->cstat |= kWallTransluc;
			}
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			spritetype* pSpr = &sprite[j];
			pSpr->hitag = pSpr->lotag = pSpr->detail = 0;
			pSpr->owner = -1;
			
			if (bloodMap)
			{
				if (pSpr->extra > 0)
					dbDeleteXSprite(pSpr->extra);
			}
			else
			{
				// cleanup cstat
				cstat = pSpr->cstat;		pSpr->cstat = 0;
				if (cstat & kSprBlock)		pSpr->cstat |= kSprBlock;
				if (cstat & kSprTransluc1)	pSpr->cstat |= kSprTransluc1;
				if (cstat & kSprFlipX)		pSpr->cstat |= kSprFlipX;
				if (cstat & kSprFlipY)		pSpr->cstat |= kSprFlipY;
				if (cstat & kSprOneSided)	pSpr->cstat |= kSprOneSided;
				switch(cstat & kSprRelMask)
				{
					case kSprFace: 			pSpr->cstat |= kSprFace; 		break;
					case kSprWall: 			pSpr->cstat |= kSprWall; 		break;
					case kSprFloor: 		pSpr->cstat |= kSprFloor; 		break;
				}
				if (cstat & kSprOrigin)		pSpr->cstat |= kSprOrigin;
				if (cstat & kSprHitscan)	pSpr->cstat |= kSprHitscan;
				if (cstat & kSprInvisible)	pSpr->cstat |= kSprInvisible;
			}
		}
	}

	eraseExtra();
}

void IMPORT_WIZARD::MapAdjustShade(int nPalookus)
{
	// Blood has 64 palookups, so treat it as 100%
	// Generic Build games mostly using 32, but
	// Exhumed has 16
	
	int perc = (nPalookus * 100) / 64;
	int i, j, swal, ewal;

	for (i = 0; i < numsectors; i++)
	{
		sector[i].ceilingshade = (schar)ClipRange(sector[i].ceilingshade + perc2val(sector[i].ceilingshade, perc), -64, 63);
		sector[i].floorshade   = (schar)ClipRange(sector[i].floorshade   + perc2val(sector[i].floorshade,   perc), -64, 63);
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
			wall[j].shade = (schar)ClipRange(wall[j].shade + perc2val(wall[j].shade, perc), -64, 63);
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			sprite[j].shade = (schar)ClipRange(sprite[j].shade + perc2val(sprite[j].shade, perc), -64, 63);
	}
}

int IMPORT_WIZARD::MapCheckFile(IOBuffer* pIo, int* pSuppVerDB, int DBLen, BOOL* bloodMap)
{
	return dbCheckMap(pIo, pSuppVerDB, DBLen, bloodMap);
}

void IMPORT_WIZARD::Init()
{
	bloodMap = 0;
	selmap = 0;
	selpal = 0;
	mapVersion = -1;
	nArtFiles = 0;
	
	pArtFiles = NULL;
	pMapData  = NULL;
	
	ArtSetStartTile();
}

void IMPORT_WIZARD::Free()
{
	if (pArtFiles)
		free(pArtFiles); 
	
	if (pMapData)
		free(pMapData);
	
	pArtFiles = NULL;
	pMapData  = NULL;
}