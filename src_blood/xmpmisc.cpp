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
#include "xmparted.h"
#include "preview.h"
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

void clampSprite(spritetype* pSprite, int which) {

    int zTop, zBot;
    if (pSprite->sectnum >= 0 && pSprite->sectnum < kMaxSectors)
	{
        GetSpriteExtents(pSprite, &zTop, &zBot);
        if (which & 0x01)
			pSprite->z += ClipHigh(getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot, 0);
		if (which & 0x02)
			pSprite->z += ClipLow(getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zTop, 0);
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

char nextEffectivePlu(int nTile, signed char nShade, unsigned char nPlu, BOOL dir, int minPerc)
{
	while( 1 )
	{
		nPlu = (unsigned char)getClosestId(nPlu, kPluMax - 1, "PLU", dir);
		if (nPlu < 2 || isEffectivePLU(nTile, palookup[nPlu]) > minPerc)
			return nPlu;
	}
}

// compare how many pixels was replaced in tile by given palookup
int isEffectivePLU(int nTile, BYTE* pDestPlu, signed char nShade, BYTE* pNormPlu, int* changed, int* total)
{
	BYTE* pTile;
	BYTE* pPluA = (BYTE*)(pNormPlu + (nShade << 8));
	BYTE* pPluB = (BYTE*)(pDestPlu + (nShade << 8));
	int l = tilesizx[nTile]*tilesizy[nTile];
	int c = 0, t = 0;

	if (total)		*total 		= 0;
	if (changed)	*changed 	= 0;
	if ((pTile = tileLoadTile(nTile)) == NULL)
		return 0;
		
	while(--l >= 0)
	{
		if (*pTile != 255)
		{
			if (pPluA[*pTile] != pPluB[*pTile]) c++;
			t++;
		}
		
		pTile++;
	}
	
	if (total)  	*total 		= t;
	if (changed)	*changed 	= c;
	return ((c * 100) / t);
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
			searchstat 		= OBJ_SPRITE;
			searchwall 		= (short) (pointhighlight & 16383);
			searchindex 	= searchwall;
			return 200;
		}

		searchstat 			= OBJ_WALL;
		searchwall 			= (linehighlight >= 0) ? linehighlight : pointhighlight;
		searchindex 		= searchwall;
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
		
		searchstat 		= OBJ_WALL;
		searchwall 		= linehighlight;
		searchindex 	= searchwall;
		return 100;
	}
	else if (sectorhighlight >= 0)
	{
		searchstat 		= OBJ_FLOOR;
		searchsector 	= sectorhighlight;
		searchindex 	= searchsector;
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

void doWallCorrection(int nWall, int* x, int* y, int shift)
{
	int nx, ny;
	GetWallNormal(nWall, &nx, &ny);
	*x = *x + (nx >> shift);
	*y = *y + (ny >> shift);
}

void dozCorrection(int* z, int zStep)
{
	*z = *z & ~zStep;
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

void avePointWall(int nWall, int* x, int* y)
{
	*x = ((wall[nWall].x + wall[wall[nWall].point2].x) >> 1);
	*y = ((wall[nWall].y + wall[wall[nWall].point2].y) >> 1);
}

void getSectorWalls(int nSect, int* swal, int *ewal)
{
	*swal = sector[nSect].wallptr;
	*ewal = *swal + sector[nSect].wallnum - 1;
}

BOOL isModernMap() {

	if (gCompat.modernMap || gModernMap) return TRUE;
	if (numsprites >= kVanillaMaxSprites || numxsprites >= kVanillaMaxXSprites) return TRUE;
	if (numsectors >= kVanillaMaxSectors || numxsectors >= kVanillaMaxXSectors) return TRUE;
	if (numwalls   >= kVanillaMaxWalls   || numxwalls   >= kVanillaMaxXWalls)   return TRUE;	
	return FALSE;
	
}

BOOL isMultiTx(short nSpr) {
	
	register int j = 0;
	if (sprite[nSpr].statnum < kStatFree && (sprite[nSpr].type == 25 || sprite[nSpr].type == 26))
	{
		for (BYTE i = 0; i < 4; i++)
			if (chlRangeIsFine(getDataOf(i, OBJ_SPRITE, nSpr))) j++;
	}
	
	return (j > 0);
}

BOOL multiTxGetRange(int nSpr, int out[4])
{
	register int j;
	XSPRITE* pXSpr = &xsprite[sprite[nSpr].extra];
	memset(out, 0, sizeof(out));
	
	if (chlRangeIsFine(pXSpr->data1) && pXSpr->data2 == 0 && pXSpr->data3 == 0 && chlRangeIsFine(pXSpr->data4))
	{
		if (pXSpr->data1 > pXSpr->data4)
		{
			j = pXSpr->data4;
			pXSpr->data4 = pXSpr->data1;
			pXSpr->data1 = j;
		}
		
		out[0] = pXSpr->data1;
		out[1] = pXSpr->data4;
		return TRUE; // ranged
	}
	
	for (j = 0; j < 4; j++) out[j] = getDataOf(j, OBJ_SPRITE, nSpr);
	return FALSE; // normal
}

BOOL multiTxPointsRx(int rx, short nSpr) {
	
	register BYTE j; int txrng[4];

	// ranged
	if (multiTxGetRange(nSpr, txrng))
		return (irngok(rx, txrng[0], txrng[1]));
		
	
	// normal
	for (j = 0; j < 4; j++)
	{
		if (rx == txrng[j])
			return TRUE;
	}
	
	return FALSE;
	
}



void printextShadow(int xpos, int ypos, short col, char* text, int font) {
	printext2(xpos, ypos, col, text, &vFonts[font], 0x01);
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
	if (sector[nSect].extra <= 0)
		return;
	
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
	
	int i, j; int sum = 0;
	
	// search for duplicated markers in the same coords
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum != kStatMarker)
			continue;
		
		sum = sprite[i].x + sprite[i].y + sprite[i].type;
		for (j = 0; j < kMaxSprites; j++)
		{			
			if (sprite[j].statnum != kStatMarker) continue;
			else if (i == j || sprite[j].owner != nSect) continue;
			else if (sum != sprite[j].x + sprite[j].y + sprite[j].type) continue;
			else if (*nIdx1 == -1)
			{
				*nIdx1 = j;
				if (sector[nSect].type != kSectorSlide && sector[nSect].type != kSectorSlideMarked)
					return;
			}
			else if (*nIdx2 == -1) *nIdx2 = j;
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
	
	for (i = 0; i < len; i++)
	{
		if (noColor >= 0 && pixels[i] == noColor)
			continue;
		
		colors[pixels[i]]++;
	}
	
	
	for (i = 0; i < 256; i++)
	{
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

int setupSecrets() {
	
	register int secrets = 0, nSprite = -1;
	register int i, j, s, e;
	
	for (i = 0; i < numsectors; i++)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			if (sprite[j].extra <= 0)
				continue;
			
			switch (xsprite[sprite[j].extra].txID) {
				case 1:
					if (nSprite < 0 && sprite[nSprite].statnum < kMaxStatus) nSprite = j;
					xsprite[sprite[j].extra].txID = xsprite[sprite[j].extra].command = 0;
					break;
				case 2:
					if (xsprite[sprite[j].extra].command == 64) secrets++;
					break;
			}
		}
		
		getSectorWalls(i, &s, &e);
		for (j = s; j <= e; j++)
		{
			if (wall[j].extra <= 0)
				continue;
			
			switch (xwall[wall[j].extra].txID) {
				case 1:
					xwall[wall[j].extra].txID = xwall[wall[j].extra].command = 0;
					break;
				case 2:
					if (xwall[wall[j].extra].command == 64) secrets++;
					break;
			}
		}
		
		if (sector[i].extra <= 0)
			continue;
		
		switch (xsector[sector[i].extra].txID) {
			case 1:
				xsector[sector[i].extra].txID = xsector[sector[i].extra].command = 0;
				break;
			case 2:
				if (xsector[sector[i].extra].command == 64) secrets++;
				break;
		}
	}
		
	// must keep user sprite because it can be real decoration or something...?
	if (secrets > 0 && numsectors > 0 && nSprite < 0 && (nSprite = InsertSprite(0, 0)) >= 0)
	{
		sprite[nSprite].x = wall[sector[0].wallptr].x;
		sprite[nSprite].y = wall[sector[0].wallptr].y;
		sprite[nSprite].xrepeat = sprite[nSprite].yrepeat = 0;
		sprite[nSprite].cstat |= kSprMoveReverse;
		sprite[nSprite].cstat |= kSprInvisible;
	}
	
	if (nSprite >= 0)
	{
		j = GetXSprite(nSprite);
		
		xsprite[j].rxID 	= 7;
		xsprite[j].command	= 64 + secrets;
		xsprite[j].state 	= xsprite[j].triggerOff = xsprite[j].restState = 0;
		xsprite[j].txID  	= xsprite[j].triggerOn = 1;
	}
	
	return secrets;
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
BOOL rngok(int val, int rngA, int rngB)
{
	return (val >= rngA && val < rngB);
}
BOOL irngok(int val, int rngA, int rngB)
{
	return (val >= rngA && val <= rngB);
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

short sectCstatAdd(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		setCstat(TRUE, &sector[nSect].floorstat, cstat);
		return sector[nSect].floorstat;
	}
	else
	{
		setCstat(TRUE, &sector[nSect].ceilingstat, cstat);
		return sector[nSect].ceilingstat;
	}
}

short sectCstatRem(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		setCstat(FALSE, &sector[nSect].floorstat, cstat);
		return sector[nSect].floorstat;
	}
	else
	{
		setCstat(FALSE, &sector[nSect].ceilingstat, cstat);
		return sector[nSect].ceilingstat;
	}
}

short sectCstatToggle(int nSect, short cstat, int objType) {
	
	if (objType == OBJ_FLOOR)
	{
		sector[nSect].floorstat ^= cstat;
		return sector[nSect].floorstat;
	}
	else
	{
		sector[nSect].ceilingstat ^= cstat;
		return sector[nSect].ceilingstat;
	}
}

short sectCstatGet(int nSect, int objType)
{
	if (objType == OBJ_FLOOR) 
		return sector[nSect].floorstat;
	
	return sector[nSect].ceilingstat;
	
}

short sectCstatSet(int nSect, short cstat, int objType)
{
	if (objType == OBJ_FLOOR)
	{
		sector[nSect].floorstat = cstat;
		return sector[nSect].floorstat;
	}
	else
	{
		sector[nSect].ceilingstat = cstat;
		return sector[nSect].ceilingstat;
	}
	
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
		qsetmodeany(xdim, ydim);
	
	if (tileLoadTile(gSysTiles.sectfil))
	{
		tileFreeTile(gSysTiles.sectfil);
		tileAllocTile(gSysTiles.sectfil, xdim, ydim);
	}
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

void getWallCoords(int nWall, int* x1, int* y1, int* x2, int* y2)
{
	if (x1)	*x1 = wall[nWall].x;
	if (y1)	*y1 = wall[nWall].y;
	if (x2)	*x2 = wall[wall[nWall].point2].x;
	if (y2)	*y2 = wall[wall[nWall].point2].y;
}

int getPrevWall(int nWall)
{	
	register int swal, ewal;
	getSectorWalls(sectorofwall(nWall), &swal, &ewal);
	while(swal <= ewal)
	{
		if (wall[swal].point2 == nWall)
			return swal;
		
		swal++;
	}
	
	return -1;
}

void wallDetach(int nWall)
{
	wall[nWall].nextsector	= -1;
	wall[nWall].nextwall 	= -1;
}

BOOL isUnderwaterSector(XSECTOR* pXSect) { return pXSect->underwater; }
BOOL isUnderwaterSector(sectortype* pSect) { return (pSect->extra > 0 && isUnderwaterSector(&xsector[pSect->extra])); }
BOOL isUnderwaterSector(int nSector) { return isUnderwaterSector(&sector[nSector]); }

int formatMapInfo(char* str)
{
	int i = 0; char* tmp;
	IniFile* pIni = gPreview.pEpisode;
	
	getFilename(gPaths.maps, buffer, FALSE);
	
	i = sprintf(str, strupr(buffer));
	if (pIni && pIni->SectionExists(buffer) && (tmp = pIni->GetKeyString(buffer, "Title", "")) != "")
		i += sprintf(&str[i], " - \"%s\"", tmp);
	
	if (gMapRev)
		i += sprintf(&str[i], " rev.%d", gMapRev);
		
	if (pIni && pIni->SectionExists(buffer) && (tmp = pIni->GetKeyString(buffer, "Author", "")) != "")
		i += sprintf(&str[i]," by %s", tmp);
	
	i += sprintf(&str[i], ", format: v%d", mapversion);
	if (gModernMap)
		i += sprintf(&str[i], " with modern features");
	
	return i;
}



int collectObjectsByChannel(int nChannel, BOOL rx, OBJECT_LIST* pOut, char flags)
{
	BOOL ok, link = FALSE, unlink = FALSE;
	register int c = 0, j, s, e, f;
	register int i = numsectors;
		
	//return 0;
	
	switch (flags & 0x30)
	{
		case 0x10:	link   = TRUE;    break;
		case 0x20:	unlink = TRUE;    break;
	}
	
	while (--i >= 0)
	{
		getSectorWalls(i, &s, &e);
		for (j = s; j <= e; j++)
		{
			if (wall[j].extra > 0)
			{
				XWALL* pXObj = &xwall[wall[j].extra];
				if ((rx && pXObj->rxID == nChannel) || (!rx && pXObj->txID == nChannel))
				{
					if (link || unlink)
					{
						ok = ((f = collectObjectsByChannel(nChannel, !rx)) > 0 && (f != 1 || pXObj->rxID != pXObj->txID));
						if ((!ok && link) || (ok && unlink))
							continue;
					}
					
					if (pOut)
						pOut->Add(OBJ_WALL, j);
					
					c++;
				}
			}
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			if (sprite[j].extra > 0)
			{
				XSPRITE* pXObj = &xsprite[sprite[j].extra];
				if ((rx && pXObj->rxID == nChannel) || (!rx && ((isMultiTx(j) && multiTxPointsRx(nChannel, j)) || pXObj->txID == nChannel)))
				{
					if (link || unlink)
					{
						ok = ((f = collectObjectsByChannel(nChannel, !rx)) > 0 && (f != 1 || pXObj->rxID != pXObj->txID));
						if ((!ok && link) || (ok && unlink))
							continue;
					}
					
					if (pOut)
						pOut->Add(OBJ_SPRITE, j);
					
					c++;
				}
			}
		}
		
		if (sector[i].extra > 0)
		{
			XSECTOR* pXObj = &xsector[sector[i].extra];
			if ((rx && pXObj->rxID == nChannel) || (!rx && pXObj->txID == nChannel))
			{
				if (link || unlink)
				{
					ok = ((f = collectObjectsByChannel(nChannel, !rx)) > 0 && (f != 1 || pXObj->rxID != pXObj->txID));
					if ((!ok && link) || (ok && unlink))
						continue;
				}

				if (pOut)
					pOut->Add(OBJ_SECTOR, i);
				
				c++;
			}
		}
	}
	
	return c;
}

int getPluOf(int oType, int oIdx)
{
	switch (oType)
	{
		case OBJ_WALL:
		case OBJ_MASKED:	return wall[oIdx].pal;
		case OBJ_FLOOR:		return sector[oIdx].floorpal;
		case OBJ_CEILING:	return sector[oIdx].ceilingpal;
		case OBJ_SPRITE:	return sprite[oIdx].pal;
	}
	
	return -1;
}

int getPicOf(int oType, int oIdx) {
	
	switch (oType)
	{
		case OBJ_FLOOR:		return sector[oIdx].floorpicnum;
		case OBJ_CEILING:	return sector[oIdx].ceilingpicnum;
		case OBJ_WALL:		return wall[oIdx].picnum;
		case OBJ_MASKED:	return wall[oIdx].overpicnum;
		case OBJ_SPRITE:	return sprite[oIdx].picnum;
	}
	
	return -1;
}

int getShadeOf(int oType, int oIdx) {
	
	switch (oType)
	{
		case OBJ_WALL:
		case OBJ_MASKED:	return wall[oIdx].shade;
		case OBJ_FLOOR:		return sector[oIdx].floorshade;
		case OBJ_CEILING:	return sector[oIdx].ceilingshade;
		case OBJ_SPRITE:	return sprite[oIdx].shade;
	}
	
	return -1;
}

void setPluOf(int nPlu, int oType, int oIdx)
{
	switch (oType)
	{
		case OBJ_WALL:
		case OBJ_MASKED:	wall[oIdx].pal			= nPlu;		break;
		case OBJ_FLOOR:		sector[oIdx].floorpal	= nPlu;		break;
		case OBJ_CEILING:	sector[oIdx].ceilingpal = nPlu;		break;
		case OBJ_SPRITE:	sprite[oIdx].pal		= nPlu;		break;
	}
}

int pluPick(int nTile, int nShade, int nPlu, char* titleArg)
{
	if (gPluPrefs.classicWindow)
		return pluPickClassic(nTile, nShade, nPlu, titleArg);

	return pluPickAdvanced(nTile, nShade, nPlu, titleArg);
}

int cmpPluEfficency(PLUINFO *ref1, PLUINFO *ref2)
{
	int t;
	if ((t = ref2->efficency - ref1->efficency) == 0)
		return ref1->id - ref2->id;
		
	return t;
}

int cmpPluId(PLUINFO *ref1, PLUINFO *ref2)
{
	return ref1->id - ref2->id;
}

int pluPickAdvanced(int nTile, int nShade, int nPlu, char* titleArg)
{
	#define kBotHeight			38
	#define kTop2Height			16
	#define kPad2 				4
	#define kButH				28
	#define kWindowSize			70
	#define kMrBase				(mrUser + 1000)
	
	char tmp[64];
	PLUINFO pluInfo[kPluMax]; PLUPICK_PREFS arg;
	int nCode, i, j, wh, hg, totalPlus = 0, efficPlus = 0;
	
	Checkbox *pClassicCb, *pShadeCb, *pSortCb, *pShowAllCb;
	Panel *pBot1, *pBot2, *pTop1, *pTop2, *pTop3;
	TextButton *pFindB, *pQuitB;
	EditNumber* pPluE;
	Container* pFocus;
	PluPick* pPicker;
	Label* pResultL;
	
	Window dialog(0, 0, ClipLow(perc2val(kWindowSize, xdim), 320), ClipLow(perc2val(kWindowSize, ydim), 200), titleArg);
	hg = dialog.client->height - kBotHeight - kTop2Height;
	wh = dialog.client->width;
	
	arg.nTile		= nTile;
	arg.nShade		= (gPluPrefs.reflectShade) ? nShade : 0;
	arg.nPlu		= nPlu;
	arg.pluInfo		= pluInfo;
	arg.pluCnt		= 0;
	
	for (i = 0; i < kPluMax; i++)
	{
		if (i == 0 || (palookup[i] && palookup[i] != palookup[0]))
		{
			totalPlus++;
			
			int pixelsTotal, pixelsAlter;
			if ((j = isEffectivePLU(nTile, palookup[i], 0, palookup[kPlu0], &pixelsAlter, &pixelsTotal)) != 0) efficPlus++;
			else if (!gPluPrefs.showAll && i != nPlu && i > 2 && !j)
				continue;
			
			arg.pluInfo[arg.pluCnt].id 				= i;
			arg.pluInfo[arg.pluCnt].efficency 		= j;
			arg.pluInfo[arg.pluCnt].pixelsTotal 	= pixelsTotal;
			arg.pluInfo[arg.pluCnt].pixelsAltered 	= pixelsAlter;
			arg.pluCnt++;
		}
	}
	
	if (gPluPrefs.mostEfficentInTop)
		qsort(&arg.pluInfo[2], arg.pluCnt-2, sizeof(arg.pluInfo[0]), (int(*)(const void*,const void*))cmpPluEfficency);
	
	arg.nCols = (widescreen) ? 6 : 5;
	arg.nRows = 3;
	
	pTop1				= new Panel(dialog.left, dialog.top, perc2val(28, dialog.client->width), kTop2Height, 1, 1, 0);
	pTop3				= new Panel(pTop1->left+pTop1->width, dialog.top, perc2val(23, dialog.client->width), kTop2Height, 1, 1, 0);
	pTop2				= new Panel(pTop3->left+pTop3->width, dialog.top, perc2val(49, dialog.client->width), kTop2Height, 1, 1, 0);
	pPicker				= new PluPick(dialog.left, pTop1->top+pTop1->height, dialog.client->width, dialog.client->height-kBotHeight-(kTop2Height<<1), &arg);
	pBot1				= new Panel(dialog.left, pPicker->top+pPicker->height, pPicker->width, kBotHeight, 1, 1, 0);
	pBot2				= new Panel(dialog.left, pBot1->top+pBot1->height, dialog.client->width, kTop2Height, 1, 1, 0);

	sprintf(tmp, 		"Shade: %+d", nShade);
	pShadeCb			= new Checkbox(0, 0, gPluPrefs.reflectShade, tmp, kMrBase);
	pShadeCb->left		= (pTop1->width>>1)-(pShadeCb->width>>1);
	pShadeCb->top		= (pTop1->height>>1)-(pShadeCb->height>>1);
	
	pSortCb				= new Checkbox(0, 0, gPluPrefs.mostEfficentInTop, "Most efficient on top", kMrBase + 1);
	pSortCb->left		= (pTop2->width>>1)-(pSortCb->width>>1);
	pSortCb->top		= (pTop2->height>>1)-(pSortCb->height>>1);
	
	pShowAllCb			= new Checkbox(0, 0, gPluPrefs.showAll, "Show all", kMrBase + 2);
	pShowAllCb->left	= (pTop3->width>>1)-(pShowAllCb->width>>1);
	pShowAllCb->top		= (pTop3->height>>1)-(pShowAllCb->height>>1);
	
	wh = pBot1->width; hg = pBot1->height;
	
	pFindB				= new TextButton(0, 0, 100, kButH, "Search", mrOk);
	pFindB->left		= kPad2;
	pFindB->top			= (hg>>1)-(pFindB->height>>1);
	pFindB->fontColor	= kColorBlue;
	
	pQuitB				= new TextButton(0, 0, 70, kButH, "Quit", mrCancel);
	pQuitB->left		= pBot1->left+wh-pQuitB->width-kPad2;
	pQuitB->top			= (hg>>1)-(pQuitB->height>>1);
	pQuitB->fontColor	= kColorRed;
	
	pClassicCb			= new Checkbox(0, 0, gPluPrefs.classicWindow, "Classic view", kMrBase + 3);
	pClassicCb->left	= pBot1->left+wh-pClassicCb->width-pQuitB->width-(kPad2<<1);
	pClassicCb->top		= (hg>>1)-(pClassicCb->height>>1);
	
	pPluE				= new EditNumber(0, 0, ClipHigh(150, pClassicCb->left - (pFindB->left + pFindB->width) - (kPad2<<1)), 20, nPlu);
	pPluE->left			= pFindB->left + pFindB->width + kPad2;
	pPluE->top			= (hg>>1)-(pPluE->height>>1);
		
	sprintf(tmp, 		"%d of %d palookups are efficient for tile #%d", efficPlus, totalPlus, nTile);
	pResultL			= new Label(0, 0, tmp, kColorBlack);
	pResultL->left		= (pBot2->width>>1)-(pResultL->width>>1);
	pResultL->top		= (pBot2->height>>1)-(pResultL->height>>1);
	
	pBot1->Insert(pFindB);
	pBot1->Insert(pPluE);
	pBot1->Insert(pClassicCb);
	pBot1->Insert(pQuitB);
	
	pBot2->Insert(pResultL);
	
	pTop1->Insert(pShadeCb);
	pTop2->Insert(pSortCb);
	pTop3->Insert(pShowAllCb);
	
	dialog.Insert(pPicker);
	dialog.Insert(pBot1);
	dialog.Insert(pTop1);
	dialog.Insert(pTop3);
	dialog.Insert(pTop2);
	dialog.Insert(pBot2);
	
	dialog.left = (xdim - dialog.width)  >> 1;
	dialog.top  = (ydim - dialog.height) >> 1;

	while( 1 )
	{
		dialog.ClearFocus();
		if ((nCode = ShowModal(&dialog, kModalNoCenter)) == mrCancel)
			break;
		
		if (nCode == kMrBase)
		{
			gPluPrefs.reflectShade = pShadeCb->checked;
			arg.nShade = (gPluPrefs.reflectShade) ? nShade : 0;
			continue;
		}
		else if (nCode == kMrBase + 1)
		{
			gPluPrefs.mostEfficentInTop	= pSortCb->checked;
			qsort
			(
				&arg.pluInfo[2],
				arg.pluCnt-2,
				sizeof(arg.pluInfo[0]),
				(int(*)(const void*,const void*))((gPluPrefs.mostEfficentInTop) ? cmpPluEfficency : cmpPluId)
			);
			
			continue;
		}
		else if (nCode == kMrBase + 2)
		{
			gPluPrefs.showAll = pShowAllCb->checked;
			return pluPick(nTile, nShade, nPlu, titleArg);
		}
		else if (nCode == kMrBase + 3)
		{
			gPluPrefs.classicWindow = pClassicCb->checked;
			return pluPick(nTile, nShade, nPlu, titleArg);
		}
		else if (pBot1->focus != &pBot1->head)
		{
			pFocus = (Container*)pBot1->focus;
			if ((pFindB == (TextButton*)pFocus) || (pPluE == (EditNumber*)pFocus))
			{
				i = arg.pluCnt;
				while(--i >= 0)
				{
					if (arg.pluInfo[i].id != pPluE->value) continue;
					nPlu = pPluE->value;
					break;
				}
				
				pFindB->pressed  = FALSE;
				if (i >= 0)
					break;
				
				Alert("Palookup ID %d not found!", pPluE->value);
			}
			
			continue;
		}
		else if (dialog.focus != &dialog.head)
		{
			pFocus = (Container*)dialog.focus;
			if (pPicker == (PluPick*)pFocus->focus)
			{
				if (nCode == mrOk)
				{
					if (pPicker->nCursor >= arg.pluCnt)
						continue;
					
					nPlu = pluInfo[pPicker->nCursor].id;
				}
				else
					nPlu = pPicker->value - mrUser;
			}
		}
		
		break;
	}
	
	return nPlu;
}

int pluPickClassic(int nTile, int nShade, int nPlu, char* titleArg)
{
	int l = ClipLow(gfxGetTextLen(titleArg, pFont), 200);
	int nCode;
	
	Window dialog(0, 0, l, 55, titleArg);
	EditNumber* pEn = new EditNumber(4, 4, dialog.width-12, 16, nPlu);
	Checkbox* pAdva = new Checkbox(4, 4, gPluPrefs.classicWindow, "Classic view", mrUser + 100); 
	pAdva->left = pEn->left + pEn->width - pAdva->width - 4;
	pAdva->top  = pEn->top + pAdva->height + 6;
	
	dialog.Insert(pEn);
	dialog.Insert(pAdva);

	nCode = ShowModal(&dialog);
	if (nCode == mrUser + 100)
	{
		gPluPrefs.classicWindow = pAdva->checked;
		return pluPick(nTile, nShade, nPlu, titleArg);
	}
	
	if (nCode != mrOk)
		return nPlu;

	return pEn->value;
}

BOOL isSearchSector()
{
	return (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING);
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