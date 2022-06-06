/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Misc functions for xmapedit.
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
#include <direct.h>
#include "xmpstub.h"
#include "seq.h"
#include "editor.h"
#include "xmpmisc.h"
#include "xmpconf.h"
#include "screen.h"
#include "gui.h"
#include "tile.h"
#include "img2tile.h"
#include "editor.h"
#include "edit2d.h"
#include "xmpsnd.h"
#include "mapcmt.h"
#include "keyboard.h"
#include "xmparted.h"
#include "misc.h"
#include "db.h"


NAMED_TYPE gSuppBakFiles[] = {
	
	{kMap, kBloodMapSig}, 	// see if this file is the BLOOD MAP. Does not work for other BUILD MAP files.
	{kSeq, kSEQSig}, 		// see if this file is the BLOOD SEQ. Does not work for PowerSlave SEQ files.
	{kQav, kQavSig},		// see if this file is the BLOOD QAV.	
	
};

struct PREVIEW_PLU {
	
	int id;
	BYTE* data;
	BOOL Init(BOOL force = FALSE)
	{
		if (force || data == NULL)
		{
			for (id = MAXPALOOKUPS - 1; id >= 0; id--)
			{
				if (palookup[id] == palookup[kPlu0])
					break;
			}
			
			if (id < 0) return FALSE;
			else if (data != NULL)
				Free();
			
			data = (BYTE*)Resource::Alloc(256 * 64);
			palookup[id] = data;
		}
		
		return TRUE;
	}
	
	void Make(PALETTE pal, int gray = 0)
	{
		dassert(id >= 0);
		dassert(data != NULL);
		dassert(BuildPLU(data, pal, gray) != FALSE);
		
	}
	
	void Free()
	{
		dassert(data != NULL);
		Resource::Free(data);
		data = NULL;
	}
	
};

PREVIEW_PLU gTestPLU;

BYTE fileExists(char* filename, RESHANDLE* rffItem) {

	BYTE retn = 0;
	if (!filename) return retn;
	else if (access(filename, F_OK) >= 0) // filename on the disk?
		retn |= 0x0001;
	
	if (rffItem)
	{
		int len, i;
		char tmp[BMAX_PATH]; char *fname = NULL, *fext = NULL;
		pathSplit2(filename, tmp, NULL, NULL, &fname, &fext);
		
		if (!fname || !fext) return retn;
		else if ((len = strlen(fext)) > 0)
		{
			if (fext[0] == '.')
				fext =& fext[1];
			
			if ((*rffItem = gSysRes.Lookup(fname, fext)) != NULL) // filename in RFF?
			{
				retn |= 0x0002;
			}
			else
			{
				// fileID in RFF?
				len = strlen(fname);
				for (i = 0; i < len; i++) {
					if (fname[i] < 48 || fname[i] > 57)
						break;
				}
				
				if (i == len && (*rffItem = gSysRes.Lookup(atoi(fname), fext)) != NULL)
					retn |= 0x0004;
			}
		}
	}
	
	return retn;
}

int fileLoadHelper(char* filepath, BYTE** out, int* loadFrom)
{
	RESHANDLE pFile;
	int i, hFile, nSize = 0;
	dassert(out != NULL && *out == NULL);
	if (loadFrom)
		*loadFrom = -1;
	
	
	if ((i = fileExists(filepath, &pFile)) == 0)
		return -1;
	
	// file on the disk is the priority
	if ((i & 0x01) && (hFile = open(filepath, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) >= 0)
	{
		if ((nSize = filelength(hFile)) > 0 && (*out = (BYTE*)malloc(nSize)) != NULL)
		{
			read(hFile, *out, nSize);
			if (loadFrom)
				*loadFrom = 0x01;
		}
		
		close(hFile);
	}
	// load file from the RFF then
	else if (pFile)
	{
		if ((nSize = gSysRes.Size(pFile)) > 0 && (*out = (BYTE*)malloc(nSize)) != NULL)
		{
			memcpy(*out, (BYTE*)gSysRes.Load(pFile), nSize);
			if (loadFrom)
				*loadFrom = 0x02;
		}
	}
	
	return (*out) ? nSize : -2;
}

void updateClocks() {
	
	gFrameTicks = totalclock - gFrameClock;
	gFrameClock += gFrameTicks;
	
}


void plsWait() {
	
	splashScreen("Please, wait");
}

void splashScreen(char* text) {
	
	int i, x, y, tw = 0, th = 0;
	gfxSetColor(gStdColor[28]);
	gfxFillBox(0, 0, xdim, ydim);
	QFONT* pFont = qFonts[6];
	char buff[256];

	if (text)
	{
		y = ((ydim >> 1) - (pFont->height >> 1)) + (th >> 1);
		x = (xdim >> 1) - (gfxGetTextLen(text, pFont) >> 1);
		gfxPrinTextShadow(x, y, gStdColor[kColorRed], text, pFont);
	
		sprintf(buff, "...");
		y = ((ydim >> 1) + (pFont->height >> 2)) + (th >> 1);
		x = (xdim >> 1) - (gfxGetTextLen(buff, pFont) >> 1);
		gfxPrinTextShadow(x, y, gStdColor[kColorRed], buff, pFont);
	}
	
	pFont = qFonts[0];
	gfxSetColor(gStdColor[kColorRed]); y = 27;
	gfxHLine(y, 18, xdim - 18); y--;
	gfxHLine(y, 18, xdim - 18); y--;
	y-=(pFont->height);
	
	sprintf(buff, "BUILD [%s]", __DATE__);
	gfxPrinTextShadow(18, y, gStdColor[18], strupr(buff), pFont);
	
	gfxSetColor(gStdColor[kColorRed]); y = ydim - 27;
	gfxHLine(y, 18, xdim - 18); y++;
	gfxHLine(y, 18, xdim - 18); y++;
	y+=(pFont->height >> 1);
	
	
	sprintf(buff, "www.cruo.bloodgame.ru/xmapedit"); i = gfxGetTextLen(strupr(buff), pFont);
	gfxPrinTextShadow(xdim - i - 16, y, gStdColor[18], buff, pFont);
	showframe();
	
}

void gfxPrinTextShadow(int x, int y, int col, char* text, QFONT *pFont, int shofs) {
	if (pFont->type != kFontTypeRasterVert) gfxDrawText(x + shofs, y + shofs, gStdColor[30], text, pFont);
	else viewDrawText(x + shofs, y + shofs, pFont, text, 127);
	gfxDrawText(x, y, col, text, pFont);
}

int scanBakFile(char* file) {
	
	int i, len, retn = -1, hFile;
	char signature[kBakSignatureLength];
	if ((hFile = open(file, O_RDONLY | O_BINARY)) >= 0)
	{
		for (i = 0; i < LENGTH(gSuppBakFiles); i++)
		{
			NAMED_TYPE* pTable =& gSuppBakFiles[i];
			memset(signature, 0, sizeof(signature));
			read(hFile, signature, (len = strlen(pTable->name)));
			if (memcmp(signature, pTable->name, len) != 0)
			{
				lseek(hFile, 0, SEEK_SET);
				continue;
			}
			
			retn = pTable->id;
			break;
		}

		close(hFile);
	}
	
	return retn;
}

void Delay(int time) {
	int done = totalclock + time;
	while (totalclock < done);
}

void BeepOk(void) {

	if (!gMisc.beep) return;
	gBeep.Play(kBeepOk);
}


void BeepFail(void) {
	
	if (!gMisc.beep) return;
	gBeep.Play(kBeepFail);

}

BOOL Beep(BOOL cond) {
	
	if (cond) BeepOk();
	else BeepFail();
	return cond;
	
}

char getPLU(int nTile, char* nPlu, BOOL dir, int minPerc) {
	
	while( 1 )
	{
		*nPlu = (char)getClosestId(*nPlu, kPluMax - 1, "PLU", dir);
		if (*nPlu <= 1 || isEffectivePLU(nTile, palookup[*nPlu]) > minPerc)
			return *nPlu;
	}
}


int getClosestId(int nId, int nRange, char* nType, BOOL dir) {
	
	int id = nId; int cnt = nRange;
	while(cnt-- >= 0) {
		id = ((dir) ? IncRotate(id, nRange) : DecRotate(id, nRange));
		if (gSysRes.Lookup(id, nType)) {
			nId = id;
			break;
		}
	}
	
	return nId;
}

void clampSprite(spritetype* pSprite) {
	
	int zTop, zBot;
	if (pSprite->sectnum >= 0 && pSprite->sectnum < kMaxSectors) {
		
		// don't allow to put sprites in floors / ceilings.
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if (!(sector[pSprite->sectnum].ceilingstat & kSectParallax))
			pSprite->z += ClipLow(getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zTop, 0);
		if (!(sector[pSprite->sectnum].floorstat & kSectParallax))
			pSprite->z += ClipHigh(getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot, 0);
		
	}
	
}


void clampSpriteZ(spritetype* pSprite, int z, int which) {
	
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	if (which & 0x0001) pSprite->z += ClipLow(z - zTop, 0);
	if (which & 0x0002) pSprite->z += ClipHigh(z - zBot, 0);

} 

void clampCamera() {

	int fz, cz;
	if (cursectnum < 0)
		return;
	
	fz = getflorzofslope(cursectnum, posx, posy);
	cz = getceilzofslope(cursectnum, posx, posy);
	if (posz > fz)
		posz = fz;
	
	if (posz < cz)
		posz = cz;

}

BOOL obsoleteXObject(int otype, int oxindex) {
	
	if (otype == OBJ_WALL)
	{
		dassert(oxindex > 0 && oxindex < kMaxXWalls);
		if (gCommentMgr.IsBind(OBJ_WALL, oxindex) >= 0)
			return FALSE;
		
		XWALL pXModel; memset(&pXModel, 0, sizeof(XWALL));
		pXModel.reference = xwall[oxindex].reference;
		return (memcmp(&pXModel, &xwall[oxindex], sizeof(XWALL)) == 0);
	}
	else if (otype == OBJ_SPRITE)
	{
		dassert(oxindex > 0 && oxindex < kMaxXSprites);
		XSPRITE pXModel; memset(&pXModel, 0, sizeof(XSPRITE));
		pXModel.reference = xsprite[oxindex].reference;
		
		return (memcmp(&pXModel, &xsprite[oxindex], sizeof(XSPRITE)) == 0);
	}
	else if (otype == OBJ_SECTOR || otype == OBJ_CEILING || otype == OBJ_FLOOR)
	{
		dassert(oxindex > 0 && oxindex < kMaxXSectors);
		if (gCommentMgr.IsBind(OBJ_SECTOR, oxindex) >= 0)
			return FALSE;
		
		XSECTOR pXModel; memset(&pXModel, 0, sizeof(XSECTOR));
		pXModel.reference = xsector[oxindex].reference;
		pXModel.marker0   = xsector[oxindex].marker0;
		pXModel.marker1   = xsector[oxindex].marker1;
		
		return (memcmp(&pXModel, &xsector[oxindex], sizeof(XSECTOR)) == 0);
	}
	
	return FALSE;
}

int getDataOf(BYTE idata, short objType, short objIndex) {
	
	switch (objType) {
		case OBJ_SPRITE:
			dassert(sprite[objIndex].extra > 0 && sprite[objIndex].extra < kMaxXSprites);
			switch (idata) {
				case 0:  return xsprite[sprite[objIndex].extra].data1;
				case 1:  return xsprite[sprite[objIndex].extra].data2;
				case 2:  return xsprite[sprite[objIndex].extra].data3;
				case 3:  return xsprite[sprite[objIndex].extra].data4;
			}
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			dassert(wall[objIndex].extra > 0 && wall[objIndex].extra < kMaxXWalls);
			return xwall[wall[objIndex].extra].data;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			dassert(sector[objIndex].extra > 0 && sector[objIndex].extra < kMaxXSectors);
			return xsector[sector[objIndex].extra].data;
	}
	
	ThrowError("Wrong object type %d", objType);
	return -1;
	
}

// compare how many pixels was replaced in tile by given palookup
int isEffectivePLU(int nTile, BYTE* plu, int* changed, int* total) {

	PALETTE pal;
	BYTE *pTileS, *pTileD;
		
	if (changed) *changed = 0;
	if (total)   *total = 0;
	if ((pTileS = tileLoadTile(nTile)) == NULL)
		return 0;
	
	int i = 0, j = 0, cnt = 0; int perc = 0;
	int len = tilesizx[nTile]*tilesizy[nTile];
	pTileD = (BYTE*)Resource::Alloc(len);
	memcpy(pTileD, pTileS, len);
	
	memcpy(pal, gamepal, sizeof(PALETTE));
	while (i < 256) pal[i] = pal[plu[i++]];
	remapColors((intptr_t)pTileD, len, pal);
	
	i = 0;
	while (i < len)
	{
		if (pTileD[i] != 255)
		{
			if (pTileD[i] != pTileS[i]) j++;
			cnt++;
		}

		i++;
	}

	Resource::Free(pTileD);
	perc = ((j * 100) / cnt);

	if (changed) *changed = j;
	if (total)   *total = cnt;
	return perc;
}

short name2TypeId(char defName[256]) {

	short i = 0, j = 0, k = 0, len = 0, len2 = 0, nType = -1;
	char names[2][256], typeName[256], typeName2[256];
	memset(names, 0, sizeof(names));
	memset(typeName, 0, sizeof(typeName));
	memset(typeName2, 0, sizeof(typeName2));
		
	if (!GetStringBox("Name OR id of sprite type", defName))
		return nType;
	
	// upper case and space removal for first type
	for (i = 0, len = 0; i < strlen(defName); i++)
	{
		if (defName[i] != 32)
			typeName[len++] = (char)toupper(defName[i]);
	}

	// first lets check if it's a numeric id
	for (i = 0; i < len; i++)
	{
		if (typeName[i] < 48 || typeName[i] > 57)
			break;
	}
	
	// non-digit symbols found, try search by name
	if (i != len)
	{	
		// search by name and caption
		for (i = 0; i < spriteNamesLength; i++)
		{
			if (spriteNames[i].name)    sprintf(names[0], spriteNames[i].name);
			if (spriteNames[i].caption) sprintf(names[1], spriteNames[i].caption);
			for (k = 0; k < LENGTH(names); k++)
			{
				memset(typeName2, 0, LENGTH(typeName2));
				
				// upper case and space removal for second type
				for (j = 0, len2 = 0; j < strlen(names[k]); j++)
				{
					if (names[k][j] != 32)
						typeName2[len2++] = (char)toupper(names[k][j]);
				}
				
				if (len2 == len)
				{
					// comparison
					for (j = 0; j < len; j++)
					{
						if (typeName[j] != typeName2[j])
							break;
					}
					
					// found!
					if (j == len)
						return spriteNames[i].id;
				}
			}
		}

		Alert("Type not found for \"%s\"", defName);
		nType = name2TypeId(defName);
	}
	else // all symbols are digits, just search by typeId
	{ 
		nType = (short) ClipRange(atoi(typeName), 0, 1023);
		for (i = 0; i < spriteNamesLength; i++) {
			if (nType != spriteNames[i].id) continue;
			return nType;
		}
		
		if (!Confirm("Type %d is unnamed. Continue?", nType))
			nType = name2TypeId(defName);
	}

	return nType;
}

void eraseExtra() {
	
	int i = 0;
	for (i = 0; i < kMaxSectors; i++) 	sector[i].extra = -1;
	for (i = 0; i < kMaxWalls; i++) 	wall[i].extra = -1;
	for (i = 0; i < kMaxSprites; i++) 	sprite[i].extra = -1;
	
}

BOOL objectLockShowStatus(int x, int y) {
	
	x = 0;
	QFONT* pFont = qFonts[1];
	if (gObjectLock.type >= 0)
	{
		if (qsetmode == 200)
		{
			sprintf(buffer, "LOCKED ON %s #%d (TIME LEFT %d)", gSearchStatNames[gObjectLock.type], gObjectLock.idx, gObjectLock.time - totalclock);
			gfxPrinTextShadow(2, y, gStdColor[kColorYellow ^ h], strupr(buffer), pFont);
		}
		
		return TRUE;
	}
	
	return FALSE;
}

int getHighlightedObject() {
	
	if (qsetmode == 200)
	{
		switch (searchstat) {
			case OBJ_SPRITE:
				return 200;
			case OBJ_WALL:
			case OBJ_MASKED:
				return 100;
			case OBJ_FLOOR:
			case OBJ_CEILING:
				return 300;
		}
	}
	else if (pointhighlight >= 0)
	{
		if ((pointhighlight & 0xc000) == 16384)
		{
			searchstat = OBJ_SPRITE;
			searchwall = (short) (pointhighlight & 16383);
			return 200;
		}

		searchstat = OBJ_WALL;
		searchwall = (linehighlight >= 0) ? linehighlight : pointhighlight;
		return 100;
	}
	else if (linehighlight >= 0)
	{
/* 		if (pointhighlight >= 0 && (pointhighlight & 0xc000) == 16384)
		{
			int i = (pointhighlight & 0x3FFF);
			int nDist1, nDist2;
			BOOL wallSprite = ((sprite[i].cstat & kSprRelMask) == kSprWall);
			if (!wallSprite)
			{
				int sprx = sprite[i].x, spry = sprite[i].y, walx, waly, x, y;
				getclosestpointonwall(mousxplc, mousyplc, linehighlight, &walx, &waly);
				nDist1 = approxDist(mousxplc - sprx, mousyplc - spry);
				nDist2 = approxDist(mousxplc - walx, mousyplc - waly);
			}

			if (wallSprite || nDist1 < nDist2)
			{
				searchstat = OBJ_SPRITE;
				searchwall = (short) i;
				return 200;
			}

		} */
		
		searchstat = OBJ_WALL;
		searchwall = linehighlight;
		return 100;
	}
	else if (sectorhighlight >= 0)
	{
		searchstat = OBJ_FLOOR;
		searchsector = sectorhighlight;
		return 300;
	}
	else
	{
		searchstat = 255;
	}
	
	return 0;
	
}


char* getFilename(char* pth, char* out, BOOL addExt) {

	char tmp[_MAX_PATH]; char *fname = NULL, *ext = NULL;
	pathSplit2(pth, tmp, NULL, NULL, &fname, &ext);
	sprintf(out, fname);
	if (addExt)
		strcat(out, ext);
	
	return out;
}

char* getPath(char* pth, char* out, BOOL addSlash) {

	int i;
	char tmp[_MAX_PATH]; char *dsk = NULL, *dir = NULL;
	pathSplit2(pth, tmp, &dsk, &dir, NULL, NULL);
	_makepath(out, dsk, dir, NULL, NULL);
	
	if (addSlash)
	{
		if ((i = strlen(out)) > 0 && !slash(out[i - 1]))
			catslash(out);
	}
	
	return out;
}

void playSound(int sndId, int showMsg) {
	
	if (!sndActive)
		sndInit();
	
	if (sndId > 0)
	{
		RESHANDLE hEffect = gSoundRes.Lookup(sndId, "SFX");
		if (hEffect != NULL)
		{
			SFX *pEffect = (SFX *)gSoundRes.Load(hEffect);
			sndKillAllSounds();
			sndStartSample(sndId, pEffect->relVol, -1);
			sprintf(buffer, "%s.RAW: Volume=%03d, Rate=%05dkHz, Loop flag=%2.3s", pEffect->rawName, ClipHigh(pEffect->relVol, 255), sndGetRate(pEffect->format), onOff((pEffect->loopStart >= 0)));
		}
		else
		{
			sprintf(buffer, "Sound #%d does not exist!", sndId);
		}
	}
	else
	{
		sprintf(buffer, "You must set sound id first!");
	}
	
	switch (showMsg) {
		case 1:
			if (gMapedHud.draw)
			{
				gMapedHud.SetMsgImp(256, buffer);
				gMapedHud.PrintMessage();
				break;
			}
			// no break
		case 2:
			scrSetMessage(buffer);
			break;
	}
}

void doGridCorrection(int* x, int* y, int nGrid) {

	if ((nGrid = ClipRange(nGrid, 0, kMaxGrids)) > 0) { 
		
		*x = (*x+(1024 >> nGrid)) & (0xffffffff<<(11-nGrid));
		*y = (*y+(1024 >> nGrid)) & (0xffffffff<<(11-nGrid));
		
	}
	
}

void doWallCorrection(int nWall, int* x, int* y, int shift) {
	
	int nx, ny;
	GetWallNormal(nWall, &nx, &ny);
	*x = *x + (nx >> shift);
	*y = *y + (ny >> shift);
	
}

void dozCorrection(int* z, int zStep) {
	
	*z = *z & ~zStep;
	//*z = (*z+(1024 >> zStep)) & (0xffffffff<<(11-zStep));
	
}

void hit2pos(int* rtx, int* rty, int* rtz, int32_t clipmask) {
	
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	int hitx = 0, hity = 0, hitz = 0, x = 0, y = 0, z = 0;
	short sect = -1;
	
	if (qsetmode == 200)
	{
		x = 16384;
		y = divscale14(searchx - (xdim>>1), xdim>>1);

		RotateVector(&x, &y, ang);
		hitscan(posx, posy, posz, cursectnum,
			x, y, scaleZ(searchy, horiz),          
			&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, clipmask);
			
		*rtx = hitx;
		*rty = hity;
		if (rtz)
			*rtz = hitz;
	}
	else
	{
		int x2d, y2d;
		getpoint(searchx,searchy,&x2d,&y2d);
		updatesector(x2d, y2d, &sect);
		if (sect >= 0)
		{
			hitx = x2d; hity = y2d; hitz = getflorzofslope(sect, x2d, y2d);
		}
		
		*rtx = x2d;
		*rty = y2d;
		if (rtz)
			*rtz = hitz;
	}
}

void hit2sector(int *rtsect, int* rtx, int* rty, int* rtz, int gridCorrection, int32_t clipmask) {
	
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	int hitx = 0, hity = 0, hitz = 0, x = 0, y = 0, z = 0;

	*rtsect = -1;
	
	if (qsetmode == 200)
	{
		x = 16384;
		y = divscale14(searchx - (xdim>>1), xdim>>1);

		RotateVector(&x, &y, ang);
		hitscan(posx, posy, posz, cursectnum,
			x, y, scaleZ(searchy, horiz),          
			&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, clipmask);
	}
	else
	{
		int x2d, y2d;
		getpoint(searchx,searchy,&x2d,&y2d);
		updatesector(x2d, y2d, &sectorhighlight);
		if (sectorhighlight >= 0) {
			hitx = x2d; hity = y2d; hitz = getflorzofslope(sectorhighlight, x2d, y2d);
			hitsect = sectorhighlight;
		}
	}

	z = hitz;
	if (hitwall >= 0) {
		
		if (gridCorrection)
			doGridCorrection(&hitx, &hity, gridCorrection);

		doWallCorrection(hitwall, &hitx, &hity);
		
		hitsect = (short)sectorofwall(hitwall);
		x = hitx;
		y = hity;

	} else if (hitsprite >= 0) {

		hitsect = sprite[hitsprite].sectnum;
		x = sprite[hitsprite].x;
		y = sprite[hitsprite].y;

		if (gridCorrection)
			doGridCorrection(&x, &y, gridCorrection);

	} else if (hitsect >= 0) {
		
		if (gridCorrection)
			doGridCorrection(&hitx, &hity, gridCorrection);
		
		x = hitx;
		y = hity;

	}
	
	if (hitsect >= 0) {
		*rtsect = hitsect;
		*rtx = x;
		*rty = y;
		*rtz = z;
	} 
	
}

void avePointSector(int nSector, int *x, int *y) {
	
	int dax = 0, day = 0, startwall, endwall, j;
	getSectorWalls(nSector, &startwall, &endwall);
	for(j = startwall; j <= endwall; j++)
	{
		dax += wall[j].x;
		day += wall[j].y;
	}
	
	if (endwall > startwall)
	{
		dax /= (endwall-startwall+1);
		day /= (endwall-startwall+1);
	}
	
	*x = dax;
	*y = day;

}

void avePointWall(int nWall, int* x, int* y) {
	
	*x = ((wall[nWall].x + wall[wall[nWall].point2].x) >> 1);
	*y = ((wall[nWall].y + wall[wall[nWall].point2].y) >> 1);
	
}

void getSectorWalls(int nSect, int* swal, int *ewal)
{
	int start = sector[nSect].wallptr;
	int end = start + sector[nSect].wallnum - 1;
	*swal = start; *ewal = end;
}

BOOL isModernMap() {

	if (gCompat.modernMap || gModernMap) return TRUE;
	if (numsprites >= kVanillaMaxSprites || numxsprites >= kVanillaMaxXSprites) return TRUE;
	if (numsectors >= kVanillaMaxSectors || numxsectors >= kVanillaMaxXSectors) return TRUE;
	if (numwalls   >= kVanillaMaxWalls   || numxwalls   >= kVanillaMaxXWalls)   return TRUE;	
	return FALSE;
	
}

BOOL isMultiTx(short nSpr) {
	
	BYTE j = 0;
	if (sprite[nSpr].statnum < kStatFree && (sprite[nSpr].type == 25 || sprite[nSpr].type == 26)) {
		for (BYTE i = 0; i < 4; i++)
			if (chlRangeIsFine(getDataOf(i, OBJ_SPRITE, nSpr))) j++;
	}
	
	return (j > 0);
}

BOOL multiTxPointsRx(int rx, short nSpr) {
	
	BYTE j;
	XSPRITE* pXSpr = &xsprite[sprite[nSpr].extra];
	
	
	// ranged tx
	if (chlRangeIsFine(pXSpr->data1) && pXSpr->data2 == 0 && pXSpr->data3 == 0 && chlRangeIsFine(pXSpr->data4)) {
		
		if (pXSpr->data1 > pXSpr->data4) {
			
			int tmp = pXSpr->data4;
			pXSpr->data4 = pXSpr->data1;
			pXSpr->data1 = tmp;
			
		}
		
		return (rx >= pXSpr->data1 && rx <= pXSpr->data4);
		
	}
	
	// normal tx
	for (j = 0; j < 4; j++)
		if (rx == getDataOf(j, OBJ_SPRITE, nSpr))
			break;

	return (j < 4);
	
}

void printextShadow(int xpos, int ypos, short col, char* text, int font) {
	printext2(xpos, ypos, col, text, &vFonts[font], 0x01);
	//printext(xpos+1, ypos+1, gStdColor[31], -1, text, font); // shadow
	//printext(xpos, ypos, col, -1, text, font); // text
}

void printextShadowL(int xpos, int ypos, short col, char* text) {
	printextShadow(xpos, ypos, col, text, 0);
}

void printextShadowS(int xpos, int ypos, short col, char* text) {
	printextShadow(xpos, ypos, col, text, 1);
}

void findSectorMarker(int nSect, int *nIdx1, int *nIdx2) {
	
	dassert(nSect >= 0 && nSect < kMaxSectors);
	
	*nIdx1 = *nIdx2 = -1;
	if (sector[nSect].extra <= 0) return;
	switch (sector[nSect].type) {
		default: return;
		case kSectorTeleport:
		case kSectorSlideMarked:
		case kSectorRotateMarked:
		case kSectorSlide:
		case kSectorRotate:
		case kSectorRotateStep:
			break;
	}
	
	int k = 0, z = 0; int sum1 = 0;
	// search for duplicated markers in the same coords
	for (k = 0; k < kMaxSprites; k++) {
		if (sprite[k].statnum != kStatMarker)
			continue;
		
		// first found
		sum1 = sprite[k].x + sprite[k].y + sprite[k].type;
		
		// search for others
		for (z = 0; z < kMaxSprites; z++) {
			if (z == k || sprite[z].statnum != kStatMarker || sprite[z].owner != nSect) continue;
			else if (sum1 != sprite[z].x + sprite[z].y + sprite[z].type) continue;
			else if (*nIdx1 == -1) *nIdx1 = z;
			else if (*nIdx2 == -1) *nIdx2 = z;
			
			if (*nIdx1 >= 0 && sector[nSect].type != kSectorSlide && sector[nSect].type != kSectorSlideMarked) return;
			else if (*nIdx1 >= 0 && *nIdx2 >= 0) return;
			else break;
		}

	}
}

BYTE mostUsedColor(BYTE* pixels, int len, short noColor) {
	
	if (pixels == NULL || *pixels == NULL)
		return 0;
	
	int colors[256], i = 0, j = 0, most = 0;
	memset(colors, 0, sizeof(colors));
	
	for (i = 0; i < len; i++) {
		if (noColor >= 0 && pixels[i] == noColor)
			continue;
		
		colors[pixels[i]]++;
	}
	
	
	for (i = 0; i < 256; i++) {
		if (colors[i] < most)
			continue;
		most = colors[i];
		j = i;
	}
	
	return (BYTE)j;
	
}


void doAutoGrid() {
	if (!gAutoGrid.enabled) return;
	for (int i = kMaxGrids - 1; i >= 0; i--) {
		if (!gAutoGrid.zoom[i] || zoom < gAutoGrid.zoom[i]) continue;
		grid = (short)ClipHigh(i + 1, 6);
		break;
	}
}

BOOL markerIsNode(int idx, int goal) {
	
	int cnt = 0;
	if (xsprite[sprite[idx].extra].data1 != xsprite[sprite[idx].extra].data2) {
		
		for (int i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i]) {
			if (sprite[i].index == idx) continue;
			else if (sprite[i].extra > 0 && xsprite[sprite[i].extra].data1 == xsprite[sprite[idx].extra].data2) {
				if (++cnt >= goal)
					return TRUE;
			}
		}
		
	}
	
	return FALSE;
}

void setupSecrets() {
	
	int i = 0, j = 0, nSprite = -1, secrets = 0;
	
	for (i = 0; i < numsectors; i++)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			if (sprite[j].extra <= 0 || sprite[j].statnum >= kMaxStatus) continue; 
			switch (xsprite[sprite[j].extra].txID) {
				case 1:
					if (nSprite == -1) nSprite = sprite[j].index;
					xsprite[sprite[j].extra].txID = xsprite[sprite[j].extra].command = 0;
					break;
				case 2:
					if (xsprite[sprite[j].extra].command != 64) break;
					secrets++;
					break;
			}
		}
		
		if (sector[i].extra <= 0) continue;
		switch (xsector[sector[i].extra].txID) {
			case 1:
				xsector[sector[i].extra].txID = xsector[sector[i].extra].command = 0;
				break;
			case 2:
				if (xsector[sector[i].extra].command != 64) break;
				secrets++;
				break;
		}
	}
	
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra <= 0) continue;
		switch (xwall[wall[i].extra].txID) {
			case 1:
				xwall[wall[i].extra].txID = xwall[wall[i].extra].command = 0;
				break;
			case 2:
				if (xwall[wall[i].extra].command != 64) break;
				secrets++;
				break;
		}
	}

	if (nSprite == -1)
	{
		nSprite = InsertSprite(0, 0);
		sprite[nSprite].x = wall[sector[0].wallptr].x;
		sprite[nSprite].y = wall[sector[0].wallptr].y;
		sprite[nSprite].xrepeat = sprite[nSprite].yrepeat = 0;
	}
	
	
	sprite[nSprite].cstat |= kSprMoveReverse;
	sprite[nSprite].cstat |= kSprInvisible;
	if ((j = sprite[nSprite].extra) <= 0)
		j = dbInsertXSprite(nSprite);

	
	xsprite[j].rxID = 7;
	xsprite[j].command = 64 + secrets;
	xsprite[j].state = xsprite[j].triggerOff = xsprite[j].restState = 0;
	xsprite[j].txID = xsprite[j].triggerOn = 1;
	
}

void cpyObjectClear() {
	
	memset(cpysprite, 0, sizeof(cpysprite) - sizeof(spritetype));
	memset(cpyxsprite, 0, sizeof(cpyxsprite) - sizeof(XSPRITE));
	memset(cpysector, 0, sizeof(cpysector) - sizeof(sectortype));
	memset(cpyxsector, 0, sizeof(cpyxsector) - sizeof(XSECTOR));
	memset(cpywall, 0, sizeof(cpywall) - sizeof(walltype));
	memset(cpyxwall, 0, sizeof(cpyxwall) - sizeof(XWALL));
	cpyspritecnt = cpysectorcnt = cpywallcnt = 0;
	
}

char* array2str(NAMED_TYPE* in, int inLen, char* out, int outLen) {
	
	memset(out, 0, outLen);
	
	int i, j, len;
	for (i = 0, j = 0; i < inLen; i++)
	{
		if (!in[i].name)
			continue;
		
		len = strlen(in[i].name);
		if (j + len >= outLen)
			break;

		Bstrcat(out, ".");
		Bstrcat(out, in[i].name);
		j+=(len + 1);
	}
	
	return out;
}

void gfxDrawCaption(int x, int y, int fr, int bg, int bgpad, char* txt, QFONT* pFont)
{
	int len = gfxGetTextLen(txt, pFont);
	int heigh = (pFont) ? pFont->height : 8;
	gfxSetColor(bg);
	gfxFillBox(x-bgpad, y-bgpad, x+len+bgpad, y+heigh+bgpad);
	gfxDrawText(x, y, fr, txt, pFont, FALSE);
}


void removeExtension (char *str) {
	for (int i = Bstrlen(str) - 1; i >= 0; i--)
	{
		if (str[i] != '.') continue;
		str[i] = '\0';
		break;
	}
}


// Fresh Supply needs this
void resortFree() {
	
	for (int i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum == kStatFree)
		{
			RemoveSpriteStat(i);
			InsertSpriteStat(i, kStatFree);
		}
	}
}


BOOL slash(char ch) {
	
	#ifdef _WIN32
		return ch == '\\';
	#else
		return ch == '/';
	#endif
}

char* catslash(char* str) {
	
	#ifdef _WIN32
		return strcat(str, "\\");
	#else
		return strcat(str, "/");
	#endif
}

// rough approximation of what _splitpath2() does in WATCOM
void pathSplit2(char *pzPath, char* buff, char** dsk, char** dir, char** fil, char** ext)
{
	int i = 0;
	char items[4][BMAX_PATH];
	memset(items, 0, sizeof(items));
	if (pzPath && strlen(pzPath))
		_splitpath(pzPath, items[0], items[1], items[2], items[3]);
	
	dassert(buff != NULL);
	
	if (dsk)
	{
		*dsk =& buff[i];
		i+=sprintf(&buff[i], items[0])+1;
	}
	
	if (dir)
	{
		*dir =& buff[i];
		i+=sprintf(&buff[i], items[1])+1;
	}
	
	if (fil)
	{
		*fil =& buff[i];
		i+=sprintf(&buff[i], items[2])+1;
	}
	
	if (ext)
	{
		*ext =& buff[i];
		i+=sprintf(&buff[i], items[3])+1;
	}
	
}

BOOL chlRangeIsFine(int val) {
	
	return (val > 0 && val < 1024);
}

short getSector() {
	
	int sect = -1; int x, y, z; // dummy
	hit2sector(&sect, &x, &y, &z, 0, 0);
	return (short)sect;
	
}

void TranslateWallToSector( void )
{
	switch (searchstat) {
		case OBJ_WALL:
			if ((searchsector = wall[searchwall2].nextsector) >= 0) break;
			// no break
		case OBJ_MASKED:
			searchsector = sectorofwall(searchwall);
			break;
	}
	
	if (searchwallcf)
	{
		searchstat = OBJ_FLOOR;
	}
	else
	{
		searchstat = OBJ_CEILING;
	}
}

// carefully when using this function as it intended for autoData[]
BOOL getSeqPrefs(int nSeq, short* pic, short* xr, short* yr, short* plu) {

	*pic = *xr = *yr = *plu = -1;
	Seq* pSeq = NULL; RESHANDLE hSeq = gSysRes.Lookup(nSeq, "SEQ");
	if (hSeq != NULL && (pSeq = (Seq*)gSysRes.Load(hSeq)) != NULL && pSeq->nFrames >= 0)
	{
		SEQFRAME* pFrame = &pSeq->frames[0];
		
		*pic = (short)seqGetTile(pFrame);
		if (pFrame->xrepeat) *xr  = (BYTE)pFrame->xrepeat;
		if (pFrame->yrepeat) *yr  = (BYTE)pFrame->yrepeat;
		if (pFrame->pal)     *plu = (BYTE)pFrame->pal;
		return TRUE;
	}
	
	return FALSE;
	
}

char* retnCodeCheck(int retnCode, NAMED_TYPE* errMsg) {

	int i = 0;
	if (retnCode < 0 && errMsg != NULL)
	{
		while ( 1 )
		{
			if (errMsg[i].id == -999 || errMsg[i].id == retnCode)
				return errMsg[i].name;
			
			i++;
		}
	}

	return NULL;

}

BOOL tmpFileMake(char* out) {

//#if 0
	
	// tmpnam makes file for read only wtf :(((
	int i, j, hFile = -1, retries = 1000;
	char pth[BMAX_PATH];
	
	while (retries--)
	{
		i = 0;
		j = sprintf(pth, kTempFileBase);
		while(i < 3)
		{
			pth[j++] = (char)(65 + Random(25));
			i++;
		}

		sprintf(&pth[j], "%d.tmp",  Random(99));
		if (fileExists(pth) <= 0)
		{
			if ((hFile = open(pth, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE)) >= 0) close(hFile);
			sprintf(out, pth);
			break;
		}
	}
	
	return (hFile >= 0);
	
//#else
	//tmpnam(out);
	//return (fileExists(out) && fileAttrSetWrite(out));
//#endif


}

int replaceByte(BYTE* buf, int len, BYTE valueA, BYTE valueB) {

	BYTE* pBuf = buf;
	int i = 0, rpc = 0;
	while (i < len)
	{
		if (*pBuf == valueA) *pBuf = valueB, rpc++;
		pBuf++, i++;
	}

	return rpc;
}

BOOL fileAttrSet(char* file, int attr)
{
	return (_chmod(file, attr) == 0);
}

BOOL fileAttrSetRead(char* file)
{
	return fileAttrSet(file, S_IREAD);
}

BOOL fileAttrSetWrite(char* file)
{
	return fileAttrSet(file, S_IWRITE);
}

BOOL fileDelete(char* file)
{
	if (!fileExists(file)) return TRUE;
	fileAttrSetWrite(file);
	unlink(file);
	
	return (!fileExists(file));
}

BOOL irngok(int val, int rngA, int rngB)
{
	return rngok(val, rngA, rngB+1);
}

void setCstat(BOOL enable, short* pStat, int nStat)
{
	if (enable)
	{
		if (!(*pStat & nStat))
			*pStat |= (short)nStat;
	}
	else if ((*pStat & nStat))
		*pStat &= ~(short)nStat;
}

short wallCstatAdd(int nWall, short cstat, BOOL nextWall) {
	
	setCstat(TRUE, &wall[nWall].cstat, cstat);
	if (nextWall && wall[nWall].nextwall >= 0)
		setCstat(TRUE, &wall[wall[nWall].nextwall].cstat, cstat);

	return wall[nWall].cstat;
}

short wallCstatRem(int nWall, short cstat, BOOL nextWall) {
	
	setCstat(FALSE, &wall[nWall].cstat, cstat);
	if (nextWall && wall[nWall].nextwall >= 0)
		setCstat(FALSE, &wall[wall[nWall].nextwall].cstat, cstat);
	
	return wall[nWall].cstat;
}

short wallCstatToggle(int nWall, short cstat, BOOL nextWall) {
	
	wall[nWall].cstat ^= cstat;
	if (nextWall && wall[nWall].nextwall >= 0)
	{
		if ((wall[nWall].cstat & cstat))
		{
			if (!(wall[wall[nWall].nextwall].cstat & cstat))
				wallCstatAdd(wall[nWall].nextwall, cstat, FALSE);
		}
		else
		{
			if ((wall[wall[nWall].nextwall].cstat & cstat))
				wallCstatRem(wall[nWall].nextwall, cstat, FALSE);
		}
	}
	
	return wall[nWall].cstat;
}

int perc2val(int reqPerc, int val) {
	
	return (val*reqPerc) / 100;
	
}

void swapValues(int *nSrc, int *nDest)
{
	int nTmp;
	nTmp = *nSrc, *nSrc = *nDest, *nDest = nTmp;
}

int scaleZ(int sy, int hz)
{
	// i don't know why hitscan is buggy on widescreen
	return (scale(sy, 200, ydim) - hz) * 2000;
}

int keyneg(int step, BYTE key, bool rvrs)
{
	if (!key)
		key = keyGet();
	
	switch (key) {
		case KEY_LEFT:
		case KEY_UP:
		case KEY_MINUS:
		case KEY_PADMINUS:
		case KEY_PADLEFT:
		case KEY_PADUP:
		case KEY_PAGEUP:
		case KEY_COMMA:
		case KEY_PADSLASH:
		case KEY_PAD7:
			return (rvrs) ? abs(step) : -step;
	}
	
	return (rvrs) ? -step : abs(step);
}

int kneg(int step, bool rvrs)
{
	return keyneg(step, key, rvrs);
}

char* getExt(int nExt)
{
	return (nExt < 0 || nExt >= kExtMax) ? NULL : gExtNames[nExt];
}

void toggleResolution(int fs, int xres, int yres, int bpp)
{
	int mode = qsetmode;
	scrSetGameMode(fs, xres, yres, bpp);
	if (mode != 200)
		qsetmodeany(xres, yres);
	
	if (tileLoadTile(gSysTiles.sectfil))
	{
		tileFreeTile(gSysTiles.sectfil);
		tileAllocTile(gSysTiles.sectfil, xres, yres);
	}
	gfxSetClip(0, 0, xdim, ydim);
}

int DlgSaveChanges(char* text, BOOL askForArt) {

	int retn = -1;

	// create the dialog
	int len = ClipRange(gfxGetTextLen(text, pFont) + 10, 202, xdim - 5);
	Window dialog(59, 80, len, ydim, text);

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrOk));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
 	dialog.Insert(new TextButton(132, 4, 60, 20, "&Cancel", mrCancel));
	
	Checkbox* pArt = NULL;
	if (askForArt)
	{
		pArt = new Checkbox(4, 30, TRUE, "With ART changes.");
		pArt->canFocus = FALSE;
		dialog.Insert(pArt);
		dialog.height = 64;
	}
	else
		dialog.height = 46;
	
	ShowModal(&dialog);
	if (dialog.endState == mrOk && dialog.focus)	// find a button we are focusing on
	{
		Container* pCont = (Container*)dialog.focus;
		TextButton* pFocus = (TextButton*)pCont->focus;
		if (pFocus)
			retn = pFocus->result;
	}
	else
	{
		retn = dialog.endState;
	}
	
	switch(retn) {
		case mrOk:
			retn = (pArt && pArt->checked) ? 2 : 1;
			break;
		case mrNo:
			retn = 0;
			break;
		case mrCancel:
			retn = -1;
			break;
	}
	return retn;

}


int countUniqBytes(BYTE* pBytes, int len)
{
	int i = 0;
	char bytes[256];
	memset(bytes, 0, sizeof(bytes));
	
	while(len--)
	{
		if (bytes[pBytes[len]]) continue;
		bytes[pBytes[len]] = 1;
		i++;
	}
	
	return i;
}

void* getFuncPtr(FUNCT_LIST* db, int dbLen, int nType)
{
	while(dbLen--)
	{
		if (db[dbLen].funcType == nType)
			return db[dbLen].pFunc;
	}
	
	return NULL;
}

int getTypeByExt(char* str, NAMED_TYPE* db, int len) {
	
	int i; char* fext; char tmp[_MAX_PATH];
	pathSplit2(str, tmp, NULL, NULL, NULL, &fext);
	if (fext)
	{
		if (fext[0] == '.')
			fext =& fext[1];
		
		while(len--)
		{
			if (stricmp(db[len].name, fext) == 0)
				return db[len].id;
		}
	}

	return -1;
	
}

BOOL isSkySector(int nSect, int nFor)
{
	if (nFor == OBJ_CEILING)
		return (BOOL)(sector[nSect].ceilingstat & kSectParallax);
	
	return (BOOL)(sector[nSect].floorstat & kSectParallax);
}

BOOL ceilPicMatch(int nSect, int nPic)
{
	return (sector[nSect].ceilingpicnum == nPic);
}

BOOL floorPicMatch(int nSect, int nPic)
{
	return (sector[nSect].floorpicnum == nPic);
}

BOOL makeBackup(char* filename)
{
	if (!fileExists(filename))
		return TRUE;
	
	char temp[_MAX_PATH];
	sprintf(temp, filename);
	ChangeExtension(temp, ".bak");
	if (fileExists(temp))
		unlink(temp);
	
	return (rename(filename, temp) == 0);
}

/* void getSpriteExtents2(spritetype* pSpr, int* x1, int* y1)
{
	int nAng, i;
	i = mulscale6(tilesizx[pSpr->picnum], pSpr->xrepeat);
	*x1 = pSpr->x + ((sintable[(pSpr->ang + kAng180) & kAngMask] * i) / 0x1000);
	*y1 = pSpr->y + ((sintable[(pSpr->ang + kAng90)  & kAngMask] * i) / 0x1000);
} */

/* 

int getPicOf(int oType, int oIdx) {
	
	switch (oType) {
		case OBJ_FLOOR:
			return sector[oIdx].floorpicnum;
		case OBJ_CEILING:
			return sector[oIdx].ceilingpicnum;
		case OBJ_WALL:
			return wall[oIdx].picnum;
		case OBJ_MASKED:
			return wall[oIdx].overpicnum;
		case OBJ_SPRITE:
			return sprite[oIdx].picnum;
	}
	
	return -1;
}

void setPicOf(int oType, int oIdx, int nPic) {
	
	switch (oType) {
		case OBJ_FLOOR:
			sector[oIdx].floorpicnum = nPic;
			break;
		case OBJ_CEILING:
			sector[oIdx].ceilingpicnum = nPic;
			break;
		case OBJ_WALL:
			wall[oIdx].picnum = nPic;
			break;
		case OBJ_MASKED:
			wall[oIdx].overpicnum = nPic;
			break;
		case OBJ_SPRITE:
			sprite[oIdx].picnum = nPic;
			break;
	}
	
	return;
	
}

BOOL ss2obj(int* objType, int* objIdx, BOOL asIs) {
	
	*objType = *objIdx = -1;
	
	switch (searchstat) {
		case OBJ_FLOOR:
		case OBJ_CEILING:
			*objType = (asIs) ? searchstat : OBJ_SECTOR;
			*objIdx  = searchsector;
			return TRUE;
		case OBJ_WALL:
		case OBJ_MASKED:
			*objType = (asIs) ? searchstat : OBJ_WALL;
			*objIdx  = (asIs) ? searchwall : searchwall2;
			return TRUE;
		case OBJ_SPRITE:
			*objType = searchstat;
			*objIdx  = searchwall;
			return TRUE;
	}
	
	return FALSE;
}

BOOL dosboxRescan() {
	
	// system() makes DosBoxECE crash when launching xmapedit
	// from bat file for some reason :(
	// i have to use hacks to force clear drive cache
	int hFile;
	if ((hFile = open(kRescanFile, O_WRONLY | O_CREAT)) >= 0)
	{
		close(hFile);
		unlink(kRescanFile);
		return TRUE;
	}
	
	return FALSE;
}

void pathMake(char* out, char* dsk, char* dir, char* fil, char* ext) {
	
	char tmp[BMAX_PATH];
	memset(tmp, 0, sizeof(tmp));
	
	int len;
	int dl = (dir) ? Bstrlen(dir) : 0;
	int fl = (fil) ? Bstrlen(fil) : 0;
	int el = (ext) ? Bstrlen(ext) : 0;

	if (dl)
	{
		Bstrcat(tmp, dir);
		len = ClipLow(Bstrlen(tmp) - 1, 0);
		if (fl && !slash(tmp[len]))
			catslash(tmp);
	}
	
	if (fl)
	{
		Bstrcat(tmp, fil);
		len = ClipLow(Bstrlen(tmp) - 1, 0);
		if (el && tmp[len] != '.')
			Bstrcat(tmp, ".");
	}
	
	if (el)
	{
		len = ClipLow(Bstrlen(tmp) - 1, 0);
		if (tmp[len] != '.')
			Bstrcat(tmp, ".");
		
		if (ext[0] == '.') Bstrcat(tmp, &ext[1]);
		else Bstrcat(tmp, ext);
	}
	
	Bsprintf(out, tmp);
} */