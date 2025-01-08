/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to work with xmapedit prefab files of any case.
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
#include "prefabs.h"
#include "xmpmaped.h"
#include "aadjust.h"
#include "tile.h"
#include "hglt.h"
#include "xmparted.h"
#include "xmpview.h"

// offsets for enumStr
enum {
kPrefabSprBase 			= 0,
kPrefabSprRelx			= kPrefabSprBase,
kPrefabSprRely,
kPrefabSprRelz,
kPrefabSprPic,
kPrefabSprPal,
kPrefabSprShade,
kPrefabSprSlope,
kPrefabSprXrep,
kPrefabSprYrep,
kPrefabSprXofs,
kPrefabSprYofs,
kPrefabSprClipdist,
kPrefabSprAng,
kPrefabSprCstat,
kPrefabSprStat,
kPrefabSprHitag,
kPrefabSprLotag,
kPrefabSprRxID,
kPrefabSprTxID,
kPrefabSprState,
kPrefabSprRestState,
kPrefabSprCommand,
kPrefabSprTrigger,
kPrefabSprBusyTime,
kPrefabSprWaitTime,
kPrefabSprData1,
kPrefabSprData2,
kPrefabSprData3,
kPrefabSprData4,
kPrefabSprPhysTriggers,
kPrefabSprTriggerFlags,
kPrefabSprItem,
kPrefabSprRespawn,
kPrefabSprKey,
kPrefabSprWave,
kPrefabSprDudeFlags,
kPrefabSprLaunchLev,
kPrefabSprLaunchMode,
kPrefabSprMax,
};

// offsets for enumStr
enum {
kPrefabBoxSizeX = 0,
kPrefabBoxSizeY	= 1,
kPrefabBoxSizeZ = 2,
};

// offsets for enumStr
enum {
kPrefabBoxOffsetAboveCeil  = 0,
kPrefabBoxOffsetBelowFloor = 1,
};

short pfbAng2FaceSpr(int ang)
{
	short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
	hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);
	if (ls >= 0) {
		
		switch (ang) {
			case 0:    return rs;
			case 512:  return ts;
			case 1024: return ls;
			case 1536: return bs;
		}
		
	}
	
	return -1;
}

int pfbAdd(char *filename, int nFaceAng, int nThumbTile)
{
	char section[256], data[256], *pData;
	spritetype* pSpr; XSPRITE* pXSpr;
	
	int i, j, k, total = 0;
	int x, y, z, left, right, top, bottom, ztop, zbottom, ztofs, zbofs;
	hgltSprGetEdges(&left, &right, &top, &bottom, &ztop, &zbottom, &ztofs, &zbofs);	

	if (fileExists(filename) && unlink(filename) != 0)
		return -1;

	// get average point of sprites
	x = left + ((right - left) >> 1);
	y = top  + ((bottom - top) >> 1);
	z = ztop + ((zbottom - ztop) >> 1);

	IniFile* pfbFile = new IniFile(filename);
	pfbFile->PutKeyInt(kPrefabIniSection, "Version", kPrefabVersion);
	
	sprintf(data, "(%d,%d,%d)", abs(right - left), abs(bottom - top), abs(zbottom - ztop));
	pfbFile->PutKeyString(kPrefabIniSection, "BoxSize", data);
	
	sprintf(data, "(%d,%d,%d,%d,%d,%d)", ztofs, zbofs, 0, 0, 0, 0);
	pfbFile->PutKeyString(kPrefabIniSection, "Offsets", data);
	
	if (nFaceAng == kAng360) nFaceAng = -1;
	pfbFile->PutKeyInt(kPrefabIniSection, "FaceAngle", nFaceAng);
		
	for (i = 0; i < highlightcnt; i++)
	{
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = highlight[i] & 16383;
		if (!hgltSprIsFine(j))
			continue;
		
		pSpr = &sprite[j];
		
		int basicInfo[] =
		{
			pSpr->x - x,					pSpr->y - y,
			pSpr->z - z,					pSpr->picnum,
			pSpr->pal,						pSpr->shade,
			spriteGetSlope(pSpr->index),	pSpr->xrepeat,
			pSpr->yrepeat,					pSpr->xoffset,
			pSpr->yoffset,					pSpr->clipdist,
			pSpr->ang,						pSpr->cstat,
			pSpr->statnum,					pSpr->flags,
			pSpr->type
		};
		
		pData = data; pData += sprintf(pData, "(");
		for (k = 0; k < LENGTH(basicInfo); k++)
			pData += sprintf(pData, "%d,", basicInfo[k]);
		
		pData--;
		if (pSpr->extra > 0 && !obsoleteXObject(OBJ_SPRITE, pSpr->extra))
		{
			pXSpr = &xsprite[pSpr->extra];
			
			int going = 0;
			if (pXSpr->triggerOn)  			going |= 0x0001;
			if (pXSpr->triggerOff) 			going |= 0x0002;
			
			int tflags = 0;
			if (pXSpr->triggerPush)			tflags |= 0x0001;
			if (pXSpr->triggerVector)		tflags |= 0x0002;
			if (pXSpr->triggerImpact)		tflags |= 0x0004;
			if (pXSpr->triggerPickup)		tflags |= 0x0008;
			if (pXSpr->triggerTouch)		tflags |= 0x0010;
			if (pXSpr->triggerSight)		tflags |= 0x0020;
			if (pXSpr->triggerProximity)	tflags |= 0x0040;
			
			int tflags2 = 0;
			if (pXSpr->locked)				tflags2 |= 0x0001;
			if (pXSpr->dudeLockout)			tflags2 |= 0x0002;
			if (pXSpr->triggerOnce)			tflags2 |= 0x0004;
			if (pXSpr->decoupled)			tflags2 |= 0x0008;
			if (pXSpr->interruptable)		tflags2 |= 0x0010;
			
			int dflags = 0;
			if (pXSpr->dudeFlag4)			dflags |= 0x0001;
			if (pXSpr->dudeDeaf)			dflags |= 0x0002;
			if (pXSpr->dudeGuard)			dflags |= 0x0004;
			if (pXSpr->dudeAmbush)			dflags |= 0x0008;
			if (pXSpr->unused1)				dflags |= 0x0010;

			int launch = 0;
			if (pXSpr->lS)					launch |= 0x0001;
			if (pXSpr->lB)					launch |= 0x0002;
			if (pXSpr->lC)					launch |= 0x0004;
			if (pXSpr->lT)					launch |= 0x0008;
			
			int extraInfo[] =
			{
				(int)pXSpr->rxID,			(int)pXSpr->txID,
				(int)pXSpr->state,			(int)pXSpr->restState,
				(int)pXSpr->command,		(int)going,
				(int)pXSpr->busyTime,		(int)pXSpr->waitTime,
				(int)pXSpr->data1,			(int)pXSpr->data2,
				(int)pXSpr->data3,			(int)pXSpr->data4,
				(int)tflags,				(int)tflags2,
				(int)pXSpr->dropItem,		(int)pXSpr->respawn,
				(int)pXSpr->key,			(int)pXSpr->wave,
				(int)dflags,				(int)pXSpr->lSkill,
				(int)launch
			};
			
			for (k = 0; k < LENGTH(extraInfo); k++)
				pData += sprintf(pData, ",%d", extraInfo[k]);
		}
			
		pData += sprintf(pData, ")");
		sprintf(section, "Sprite#%05d", total++);
		pfbFile->PutKeyString(kPrefabIniSection, section, data);
	}
	
	// attach thumbnail
	if (nThumbTile >= 0 && tileLoadTile(nThumbTile) != NULL)
		pfbAttachThumbnail(pfbFile, nThumbTile);
	
	pfbFile->Save();
	delete pfbFile;
	return total;
	
}

int pfbAttachThumbnail(IniFile* pfbFile, int nThumbTile)
{
	BYTE* pTile = tileLoadTile(nThumbTile);
	const int kMaxCharsInLine = 128;
	char* section = "Thumbnail";
	int wh, hg, extraInfo[2];
	int nPos = 0, t = 0;
	char data[256];
	
	if (pfbFile->SectionExists(section))
		pfbFile->SectionRemove(section);
	
	wh = tilesizx[nThumbTile];
	hg = tilesizy[nThumbTile];
	
	extraInfo[0] = wh;
	extraInfo[1] = hg;
	
	BIN2TXT b2t;
	b2t.inf.ptr = extraInfo;
	b2t.inf.len = LENGTH(extraInfo);
	b2t.inf.flg = B2T_ENC_000F|B2T_ENC_BREP|B2T_ENC_MODS;
	
	b2t.bin.len = wh*hg;
	b2t.bin.ptr = pTile;
	
	// convert raw data into textual representation
	if (b2t.Encode())
	{
		// have to split result by lines to stay compatible
		// with old INI parser, otherwise a crash will occur
		while(nPos < b2t.txt.len)
		{
			t = 0;
			while(nPos < b2t.txt.len && t < kMaxCharsInLine)
			{
				data[t] = b2t.txt.ptr[nPos];
				nPos++;
				t++;
			}
			
			data[t] = '\0';
			pfbFile->KeyAddNew(section, data, NULL);
		}
		
		free(b2t.txt.ptr);
	}
	
	return nPos;
}

char pfbGetSize(IniFile* pfbFile, int* width, int* height, int* zHeight)
{
	char buf1[256];
	if (sprintf(buf1, pfbFile->GetKeyString(kPrefabIniSection, "BoxSize", "")))
	{
		if (width)   *width		= enumStrGetInt(kPrefabBoxSizeX, buf1);
		if (height)  *height	= enumStrGetInt(kPrefabBoxSizeY);
		if (zHeight) *zHeight 	= enumStrGetInt(kPrefabBoxSizeZ);
		return 1;
	}
	
	return 0;
}

int pfbInsert(IniFile* pfbFile, int nStat, int nID, int nSect, int x, int y, int z, int camZ)
{
	char buf1[256], buf2[256], pathMarkerFound = 0, rorMarkerFound = 0, channelsFound = 0;
	int nSpr, nXSpr = -1, ofsAboveCeil, ofsBelowFloor, var;
	int width = 0, height = 0, fspr, fang, i;
	spritetype* pSpr;
	
	if (nSect < 0) return -1;
	if (pfbFile->GetKeyInt(kPrefabIniSection, "Version", 0) != kPrefabVersion) return -3;
	if (!pfbGetSize(pfbFile, &width, &height, NULL))
		return -2;
	
	sprintf(buf1, pfbFile->GetKeyString(kPrefabIniSection, "Offsets", ""));
	ofsAboveCeil  = enumStrGetInt(kPrefabBoxOffsetAboveCeil, buf1);
	ofsBelowFloor = enumStrGetInt(kPrefabBoxOffsetBelowFloor);
	
	// face side of the prefab
	fang = (short)pfbFile->GetKeyInt(kPrefabIniSection, "FaceAngle", -1);

	switch (nStat)
	{
		case OBJ_SPRITE:
			x = sprite[nID].x;
			y = sprite[nID].y;
			z = sprite[nID].z;
			if (qsetmode != 200) // always put it on top of floor sprite in 2d mode
				camZ = sprite[nID].z - 1;
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			doGridCorrection(&x, &y, 5);
			doWallCorrection(nID, &x, &y);
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
			doGridCorrection(&x, &y, grid);
			break;
	}

	hgltReset(kHgltPoint);
	highlightcnt = i = 0;

	while( 1 )
	{
		sprintf(buf1, "Sprite#%05d", i);
		if (!sprintf(buf2, pfbFile->GetKeyString(kPrefabIniSection, buf1, "")))
			break;

		nSpr = InsertSprite((short)nSect, (short)ClipRange(enumStrGetInt(kPrefabSprStat, buf2), 0, kStatFree - 1));
		if (nSpr < 0 || numsprites >= kMaxSprites - 1)
			return -4;
		
		pSpr = &sprite[nSpr];
		
		pSpr->x = x + enumStrGetInt(kPrefabSprRelx);
		pSpr->y = y + enumStrGetInt(kPrefabSprRely);
		pSpr->z = z + enumStrGetInt(kPrefabSprRelz);

		pSpr->picnum  	= (short)ClipRange(enumStrGetInt(kPrefabSprPic),  0, kMaxTiles - 1);
		if (pSpr->picnum < 0 || !tileLoadTile(pSpr->picnum))
			pSpr->picnum = 0;
		
		pSpr->pal	  	= (char) 			ClipRange(enumStrGetInt(kPrefabSprPal), 0, 255);
		pSpr->shade	  	= (signed char) 	ClipRange(enumStrGetInt(kPrefabSprShade), -128, 63);
		pSpr->xrepeat 	= (char) 			ClipRange(enumStrGetInt(kPrefabSprXrep), 4, 255);
		pSpr->yrepeat 	= (char)			ClipRange(enumStrGetInt(kPrefabSprYrep), 4, 255);
		pSpr->xoffset 	= (signed char)  	ClipRange(enumStrGetInt(kPrefabSprXofs),  -128, 127);
		pSpr->yoffset 	= (signed char)  	ClipRange(enumStrGetInt(kPrefabSprYofs),  -128, 127);
		pSpr->ang	  	= (short)			ClipRange(enumStrGetInt(kPrefabSprAng), -32768, 32768);
		pSpr->clipdist 	= (char)			ClipRange(enumStrGetInt(kPrefabSprClipdist), 0, 255);
		pSpr->flags 	= (short)			ClipRange(enumStrGetInt(kPrefabSprHitag), -32768, 32768);
		pSpr->cstat	 	= (short)  			enumStrGetInt(kPrefabSprCstat);
		pSpr->type 		= (short)			ClipRange(enumStrGetInt(kPrefabSprLotag), 0, 1023);
	
		if ((pSpr->cstat & kSprRelMask) == kSprSloped)
			spriteSetSlope(nSpr, (short)ClipRange(enumStrGetInt(kPrefabSprSlope), -32768, 32767));
		
		// test if sprite is an xsprite
		if (enumStrGetInt(kPrefabSprRxID, NULL, ',', -1) >= 0)
		{
			// setup xsprite
			nXSpr = GetXSprite(nSpr);
			XSPRITE* pXSpr = &xsprite[nXSpr];
			
			pXSpr->rxID 			= ClipRange(enumStrGetInt(kPrefabSprRxID),  0, 1023);
			pXSpr->txID 			= ClipRange(enumStrGetInt(kPrefabSprTxID),  0, 1023);
			pXSpr->state 			= ClipRange(enumStrGetInt(kPrefabSprState), 0, 1);
			pXSpr->command 			= ClipRange(enumStrGetInt(kPrefabSprCommand), 0, 255);

			var 					= ClipRange(enumStrGetInt(kPrefabSprTrigger), 0, 3);
			if (var & 0x0001) 		pXSpr->triggerOn  = 1;
			if (var & 0x0002) 		pXSpr->triggerOff = 1;
			
			pXSpr->busyTime			= ClipRange(enumStrGetInt(kPrefabSprBusyTime), 0, 4095);
			pXSpr->waitTime			= ClipRange(enumStrGetInt(kPrefabSprWaitTime), 0, 4095);
			
			pXSpr->data1 			= ClipRange(enumStrGetInt(kPrefabSprData1), -32767, 32767);
			pXSpr->data2 			= ClipRange(enumStrGetInt(kPrefabSprData2), -32767, 32767);
			pXSpr->data3 			= ClipRange(enumStrGetInt(kPrefabSprData3), -32767, 32767);
			pXSpr->data4 			= ClipRange(enumStrGetInt(kPrefabSprData4), 0, 65535);
			
			var 					= ClipRange(enumStrGetInt(kPrefabSprPhysTriggers), 0, 127);
			if (var & 0x0001)		pXSpr->triggerPush   = 1;
			if (var & 0x0002)		pXSpr->triggerVector = 1;
			if (var & 0x0004)		pXSpr->triggerImpact = 1;
			if (var & 0x0008)		pXSpr->triggerPickup = 1;
			if (var & 0x0010)		pXSpr->triggerTouch  = 1;
			if (var & 0x0020)		pXSpr->triggerSight  = 1;
			if (var & 0x0040)		pXSpr->triggerProximity = 1;
			
			var						= ClipRange(enumStrGetInt(kPrefabSprTriggerFlags), 0, 127);
			if (var & 0x0001)		pXSpr->locked      = 1;
			if (var & 0x0002)		pXSpr->dudeLockout = 1;
			if (var & 0x0004)		pXSpr->triggerOnce = 1;
			if (var & 0x0008)		pXSpr->decoupled   = 1;
			if (var & 0x0010)		pXSpr->interruptable  = 1;
			
			pXSpr->dropItem 		= ClipRange(enumStrGetInt(kPrefabSprItem), 0, kItemMax);
			pXSpr->respawn			= ClipRange(enumStrGetInt(kPrefabSprRespawn), 0, 3);
			pXSpr->key				= ClipRange(enumStrGetInt(kPrefabSprKey), 0, 7);
			pXSpr->wave				= ClipRange(enumStrGetInt(kPrefabSprWave), 0, 3);
			
			var						= ClipRange(enumStrGetInt(kPrefabSprDudeFlags), 0, 127);
			if (var & 0x0001)		pXSpr->dudeFlag4  = 1;
			if (var & 0x0002)		pXSpr->dudeDeaf   = 1;
			if (var & 0x0004)		pXSpr->dudeGuard  = 1;
			if (var & 0x0008)		pXSpr->dudeAmbush = 1;
			if (var & 0x0010)		pXSpr->unused1 	  = 1;
			
			pXSpr->lSkill			= ClipRange(enumStrGetInt(kPrefabSprLaunchLev), 0, 127);
			var						= ClipRange(enumStrGetInt(kPrefabSprLaunchMode), 0, 127);
			if (var & 0x0001)		pXSpr->lS = 1;
			if (var & 0x0002)		pXSpr->lB = 1;
			if (var & 0x0004)		pXSpr->lC = 1;
			if (var & 0x0008)		pXSpr->lT = 1;
			
			
			if (!channelsFound)
				channelsFound = (pXSpr->rxID || pXSpr->txID);
		}
		
		if (!pathMarkerFound)
			pathMarkerFound = (pSpr->type == kMarkerPath);
		
		if (!rorMarkerFound)
			rorMarkerFound = (pSpr->type > 6 && pSpr->type <= 14 && pSpr->type != 8);
		
		hgltAdd(OBJ_SPRITE, pSpr->index);
		i++;
	}

	if (i > 0)
	{
		updatenumsprites();
		fspr = pfbAng2FaceSpr(fang);
		fang = (fang + ((fang - 1536) << 1));
		
		switch (nStat)
		{
			case OBJ_WALL:
				if (fspr >= 0) // set wall angle if correct face side is defined
				{
					pSpr = &sprite[fspr];
					hgltSprRotate((fang + (GetWallAngle(nID) + kAng90)) & kAngMask);
				}
				hgltSprPutOnWall(nID, x, y);
				break;
			case OBJ_SPRITE:
				switch (sprite[nID].cstat & kSprRelMask)
				{
					case kSprWall:
						if (fspr >= 0) // set wall sprite angle if correct face side is defined
						{
							pSpr = &sprite[fspr];
							hgltSprRotate((fang + sprite[nID].ang) & kAngMask);
							if (width || height)
								hgltSprChgXYZ(x - pSpr->x, y - pSpr->y);
						}
						break;
					case kSprFloor:
						z = sprite[nID].z;
						hgltSprPutOnZ(z, (camZ > z) ? 0x0001 : 0x0002, ofsAboveCeil, ofsBelowFloor);
						break;
					case kSprSloped:
						z = spriteGetZOfSlope(nID, x, y);
						hgltSprPutOnZ(z, (camZ > z) ? 0x0001 : 0x0002, ofsAboveCeil, ofsBelowFloor);
						break;
				}
				break;
			case OBJ_FLOOR:
				hgltSprPutOnFloor(ofsBelowFloor);
				if (fspr >= 0) hgltSprRotate(roundAngle((fang + ang + kAng180) & kAngMask, 4));
				break;
			case OBJ_CEILING:
				hgltSprPutOnCeiling(ofsAboveCeil);
				if (fspr >= 0) hgltSprRotate(roundAngle((fang + ang + kAng180) & kAngMask, 4));
				break;
			
		}
		
		hgltSprCallFunc(sprFixSector);
		AutoAdjustSprites();
		
		if (nXSpr >= 0)
		{
			if (channelsFound)   hgltIsolateChannels(kHgltPoint);
			if (pathMarkerFound) hgltIsolatePathMarkers(kHgltPoint);
			if (rorMarkerFound)  hgltIsolateRorMarkers(kHgltPoint);	
		}
		
		hgltSprClamp(ofsAboveCeil, ofsBelowFloor);
	}
	
	return i;
}

int pfbInsert(char* file, int nStat, int nID, int nSect, int x, int y, int z, int camZ)
{
	if (!fileExists(file))
		return -2;
	
	IniFile* pfbFile = new IniFile(file);
	int nRetn = pfbInsert(pfbFile, nStat, nID, nSect, x, y, z, camZ);
	delete(pfbFile);
	return nRetn;
}

int pfbDlgOptions(int* nFaceAngle, int* nThumbTile)
{
	char press1024 = 0, press0 = 0, press1536 = 1, press512 = 0;
	static char attach = 1; char colors[256];
	const int twh = 200, thg = 180, pad = 4;
	int nTile, wh, hg, mx, my, i;
	int tw, th, ts;

	FieldSet *pFaceF, *pThumbF; Panel *pFaceP, *pThumbP, *pTileP;
	BitButton2* pAng1536B, *pAng512B, *pAng0B, *pAng1024B;
	TextButton* pOkB, *pQuitB; Checkbox* pAttachC;
	Tile* pTile; Container* pFocus;
	
	*nThumbTile = -1; *nFaceAngle = -1;
	memset(colors, 0, sizeof(colors));
	
	if ((nTile = tileGetBlank()) >= 0)
	{
		if (tileAllocTile(nTile, twh, thg))
		{
			memcpy(cpysprite, 	sprite, sizeof(sprite));
			memcpy(cpywall, 	wall, 	sizeof(wall));
			h = 0;
			
			i = kMaxSprites;
			while(--i >= 0) // hide unhighlighted sprites before drawing
			{
				if (sprite[i].statnum < kMaxStatus && hgltCheck(OBJ_SPRITE, i) < 0)
					sprite[i].xrepeat = 0;
			}
			
			i = numwalls;
			while(--i >= 0) // hide masked walls so drawmasks ignore it
				wall[i].cstat &= ~kWallMasked;

			
			setviewtotile(nTile, thg, twh);
			
			// first time frame drawing to spot all the used colors
			drawrooms(posx, posy, posz, ang, horiz, cursectnum);
			viewProcessSprites(posx, posy, posz, ang);
			drawmasks();
			
			tileGetUsedColors(nTile, colors);
			
			// lookup unused one in standard greys
			// we need this to be grey to show correct
			// colors behind translucent dark sprites
			
			i = 15;
			while(i < 32 && colors[clr2std(i)])
				i++;
			
			// second drawing to fill the frame
			drawrooms(posx, posy, posz, ang, horiz, cursectnum);
			memset((void*)waloff[nTile], clr2std((i < 32) ? i : 22), twh*thg);
			viewProcessSprites(posx, posy, posz, ang);
			drawmasks();
			
			// when found unused, we can safely replace it to the transparent one!
			if (i < 32)
				replaceByte((BYTE*)waloff[nTile], twh*thg, clr2std(i), 255);
			
			setviewback();
			artedRotateTile(nTile);
			artedFlipTileY(nTile);
			
			memcpy(sprite, 	cpysprite, 	sizeof(sprite));
			memcpy(wall, 	cpywall, 	sizeof(wall));
		}
		else
		{
			nTile = -1;
		}
	}
	
	Window dialog(0, 0, 380, 240, "Prefab options");
	dialog.left = (xdim - dialog.width)  >> 1;
	dialog.top  = (ydim - dialog.height) >> 1;
	wh = dialog.client->width;
	hg = dialog.client->height;
	
	Panel* pButtons		= new Panel(pad, hg-26-(pad<<1), dialog.client->width-pad, 26);
	
	pOkB 				= new TextButton(0, 0, 90, 26,  "Confirm", mrOk);
	pOkB->fontColor 	= kColorBlue;
		
	pQuitB 				= new TextButton(0, 0, 80, 26,  "Quit", mrCancel);
	pQuitB->left 		= pOkB->left+pOkB->width+pad;
	pQuitB->fontColor 	= kColorRed;
	
	pButtons->Insert(pOkB);
	pButtons->Insert(pQuitB);
	
	pFaceF				= new FieldSet(pad, pad, (wh>>1)-(pad<<1), 170, "DEFINE FACE SIDE", kColorDarkGray, kColorDarkGray);
	pFaceP				= new Panel(pad, pad, pFaceF->width-(pad<<1), pFaceF->height-(pad<<1));
	
	pThumbF				= new FieldSet(pFaceF->left+pFaceF->width+pad, pad, (wh>>1)-pad, 170, "THUMBNAIL", kColorDarkGray, kColorDarkGray);
	pThumbP				= new Panel(pad, pad, pThumbF->width-(pad<<1), pThumbF->height-(pad<<1));
		
	pAng1536B 			= new BitButton2(66, 88, 34, 34,  pBitmaps[1], 	mrUser + 1536);
	pAng512B  			= new BitButton2(66, 24, 34, 34,  pBitmaps[0], 	mrUser + 512);
	pAng0B				= new BitButton2(14, 56, 34, 34,  pBitmaps[2], 	mrUser + 0);
	pAng1024B			= new BitButton2(122, 56, 34, 34,  pBitmaps[3],	mrUser + 1024);
	
	pAng1536B->pressed = 1;
	
	pFaceP->Insert(pAng1536B);
	pFaceP->Insert(pAng512B);	
	pFaceP->Insert(pAng0B);
	pFaceP->Insert(pAng1024B);
	
	pFaceP->Insert(new Label(70, 126, "1536"));
	pFaceP->Insert(new Label(74, 14, "512"));
	pFaceP->Insert(new Label(29, 94, "0"));
	pFaceP->Insert(new Label(126, 94, "1024"));
	
	pFaceP->Insert(new Label(59, 70, "<PREFAB>"));
	pFaceP->Insert(new Label(40, 145, "<YOU ARE HERE>"));
	pFaceP->focus = pAng1536B;
	
	pFaceF->Insert(pFaceP);

	
	
	ts = (pThumbP->width < pThumbP->height) ? pThumbP->width : pThumbP->height;
	tileDrawGetSize(nTile, ts, &tw, &th);
	
	ts = (tw < th) ? tw : th;
	mx = (pThumbP->width>>1)-(ts>>1); my = (pThumbP->height>>1)-(th>>1);

	pTile				= new Tile(pad>>1, pad>>1, nTile, ts, 0, 0, 0x02);
	pTileP				= new Panel(mx, pad, pTile->width+pad, pTile->height+pad, 0, 0, -1, -1);
	pAttachC			= new Checkbox(0, 0, attach, "Attach thumbnail", 0);
		
	pAttachC->left = (pThumbP->width>>1)-(pAttachC->width>>1);
	pAttachC->top = pTileP->top+pTileP->height+pad;
		
	pTileP->Insert(pTile);
	
	
	
	pThumbP->Insert(pTileP);
	pThumbP->Insert(pAttachC);
	pThumbF->Insert(pThumbP);

	dialog.Insert(pFaceF);
	dialog.Insert(pThumbF);

	dialog.Insert(pButtons);

	//dialog.Insert(pOkB);
	//dialog.Insert(pQuitB);

	while( 1 )
	{
		ShowModal(&dialog, kModalNoCenter);
		if (pFaceP->focus != &pFaceP->head)				pFocus = (Container*)pFaceP->focus;
		else if (pThumbP->focus != &pThumbP->head)		pFocus = (Container*)pThumbP->focus;
		else if (pButtons->focus != &dialog.head)		pFocus = (Container*)pButtons->focus;
		else pFocus = NULL;
		
		if ((dialog.endState == mrCancel) || (pQuitB == (TextButton*)pFocus))
		{
			if (nTile >= 0)
				tileFreeTile(nTile);
			
			break;
		}
		
		if (pFocus)
		{
			if (pAng1024B == (BitButton2*)pFocus)
			{
				press1024 	^= 1;
				press1536 	= 0;
				press512	= 0;
				press0 		= 0;
			}
			else if (pAng1536B == (BitButton2*)pFocus)
			{
				press1536 	^= 1;
				press1024 	= 0;
				press512	= 0;
				press0 		= 0;
			}
			else if (pAng512B == (BitButton2*)pFocus)
			{
				press512 	^= 1;
				press1536 	= 0;
				press1024	= 0;
				press0 		= 0;
			}
			else if (pAng0B == (BitButton2*)pFocus)
			{
				press0 		^= 1;
				press1536 	= 0;
				press1024	= 0;
				press512	= 0;
			}
			else if (pAttachC == (Checkbox*)pFocus)
			{
				pAttachC->checked ^= 1;
			}
			else
			{
				*nThumbTile = -1;
				if ((attach = pAttachC->checked) != 0)
				{
					*nThumbTile = nTile;
				}
				else if (nTile >= 0)
				{
					tileFreeTile(nTile);
				}

				if (pAng1536B->pressed)			*nFaceAngle = 1536;
				else if (pAng1024B->pressed)	*nFaceAngle = 1024;
				else if (pAng512B->pressed)		*nFaceAngle = 512;
				else if (pAng0B->pressed)		*nFaceAngle = 0;
				else							*nFaceAngle = -1;
				
				break;
			}
		}
		
		pAng1024B->pressed 	= press1024;
		pAng1536B->pressed 	= press1536;
		pAng512B->pressed	= press512;
		pAng0B->pressed		= press0;
	}
	
	return dialog.endState;
}