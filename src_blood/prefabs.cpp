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


#include <stdio.h>
#include <string.h>
#include <io.h>


#include "prefabs.h"
#include "gui.h"
#include "editor.h"
#include "xmpstub.h"
#include "aadjust.h"
#include "inifile.h"
#include "trig.h"
#include "tile.h"
#include "hglt.h"
#include "enumstr.h"
#include "xmpmisc.h"

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

/* CHECKBOX_LIST gPrefabType[] = {
	
	{TRUE, "For floors."},
	{TRUE, "For ceilings."},
	{TRUE, "For walls."},
	
}; */

short pfbAng2FaceSpr(int ang) {
	
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

int pfbAdd(char *filename, int faceAng) {
	
	int i = 0, j = 0, total = 0;
	int x, y, z, left = 0, right = 0, top = 0, bottom = 0, ztop = 0, zbottom = 0, ztofs = 0, zbofs = 0;
	hgltSprGetEdges(&left, &right, &top, &bottom, &ztop, &zbottom, &ztofs, &zbofs);	

	if (fileExists(filename) && !pfbRemove(filename))
		return -1;

	// get average point of sprites
	x = left + ((right - left) >> 1);
	y = top  + ((bottom - top) >> 1);
	z = ztop + ((zbottom - ztop) >> 1);

	IniFile* pfbFile = new IniFile(filename);
	pfbFile->PutKeyInt(kPrefabIniSection, "Version", kPrefabVersion);
	
	sprintf(buffer, "(%d,%d,%d)", abs(right - left), abs(bottom - top), abs(zbottom - ztop));
	pfbFile->PutKeyString(kPrefabIniSection, "BoxSize", buffer);
	
	sprintf(buffer, "(%d,%d,%d,%d,%d,%d)", ztofs, zbofs, 0, 0, 0, 0);
	pfbFile->PutKeyString(kPrefabIniSection, "Offsets", buffer);
	
	if (faceAng == kAng360) faceAng = -1;
	pfbFile->PutKeyInt(kPrefabIniSection, "FaceAngle", faceAng);
	
	for (i = 0; i < highlightcnt; i++) {
		
		if ((highlight[i] & 0xC000) == 0)
			continue;
		
		j = highlight[i] & 16383;
		if (!hgltSprIsFine(j))
			continue;

		sprintf(
			buffer2, "(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			sprite[j].x - x,
			sprite[j].y - y,
			sprite[j].z - z,
			sprite[j].picnum,
			sprite[j].pal,
			sprite[j].shade,
			spriteGetSlope(sprite[j].index),
			sprite[j].xrepeat,
			sprite[j].yrepeat,
			sprite[j].xoffset,
			sprite[j].yoffset,
			sprite[j].clipdist,
			sprite[j].ang,
			sprite[j].cstat,
			sprite[j].statnum,
			sprite[j].flags,
			sprite[j].type
		);
		
		if (sprite[j].extra > 0 && !obsoleteXObject(OBJ_SPRITE, sprite[j].extra)) {
			
			XSPRITE* pXSpr = &xsprite[sprite[j].extra];
			
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
			
			sprintf(
				buffer3, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				pXSpr->rxID,
				pXSpr->txID,
				pXSpr->state,
				pXSpr->restState,
				pXSpr->command,
				going,
				pXSpr->busyTime,
				pXSpr->waitTime,
				pXSpr->data1,
				pXSpr->data2,
				pXSpr->data3,
				pXSpr->data4,
				tflags,
				tflags2,
				pXSpr->dropItem,
				pXSpr->respawn,
				pXSpr->key,
				pXSpr->wave,
				dflags,
				pXSpr->lSkill,
				launch
			);
			
			strcat(buffer2, buffer3);
			
		}
			
		strcat(buffer2, ")");
		
		sprintf(buffer, "Sprite#%05d", total++);
		pfbFile->PutKeyString(kPrefabIniSection, buffer, buffer2);
	}
	
	pfbFile->Save();
	delete pfbFile;
	return total;
	
}

int pfbInsert(char* file, int sstat, int sect, int x, int y, int z) {

	short fspr, fang, i, nSprite;
	int nPosZ = posz, nXSprite = -1, ofsAboveCeil, ofsBelowFloor, var;
	int width = 0, height = 0, dx = 0, dy = 0;
	sstat = 0;
	
	if (sect < 0) return -1;
	else if (!fileExists(file))
		return -2;
	
	IniFile* pfbFile = new IniFile(file);
	if (!sprintf(buffer3, pfbFile->GetKeyString(kPrefabIniSection, "BoxSize", ""))) return -2;
	else if (pfbFile->GetKeyInt(kPrefabIniSection, "Version", 0) != kPrefabVersion)
		return -3;

	width   = enumStrGetInt(kPrefabBoxSizeX, buffer3);
	height  = enumStrGetInt(kPrefabBoxSizeY);

	sprintf(buffer3, pfbFile->GetKeyString(kPrefabIniSection, "Offsets", ""));
	ofsAboveCeil  = enumStrGetInt(kPrefabBoxOffsetAboveCeil, buffer3);
	ofsBelowFloor = enumStrGetInt(kPrefabBoxOffsetBelowFloor);
	
	// face side of the prefab
	fang = (short)pfbFile->GetKeyInt(kPrefabIniSection, "FaceAngle", -1);

	if (getHighlightedObject()) {

		switch (searchstat) {
			case OBJ_SPRITE:
				x = sprite[searchwall].x;
				y = sprite[searchwall].y;
				z = sprite[searchwall].z;
				if (qsetmode != 200) // always put it on top of floor sprite in 2d mode
					nPosZ = sprite[searchwall].z - 1;
				break;
			case OBJ_WALL:
			case OBJ_MASKED:
				doGridCorrection(&x, &y, 5);
				doWallCorrection(searchwall2, &x, &y);
				break;
			case OBJ_FLOOR:
			case OBJ_CEILING:
				doGridCorrection(&x, &y, grid);
				break;
		}
		
	}

	updatenumsprites();
	hgltReset(kHgltPoint);
	highlightcnt = i = 0;
	
	BOOL pathMarkerFound = FALSE;
	BOOL rorMarkerFound  = FALSE;
	BOOL channelsFound   = FALSE;

	while( 1 ) {

		sprintf(buffer, "Sprite#%05d", i);
		if (!sprintf(buffer3, pfbFile->GetKeyString(kPrefabIniSection, buffer, "")))
			break;

		nSprite = (short)InsertSprite((short)sect, (short)ClipRange(enumStrGetInt(kPrefabSprStat, buffer3), 0, kStatFree - 1));
		if (numsprites >= kMaxSprites - 1)
			return -4;


		sprite[nSprite].x 			= x + enumStrGetInt(kPrefabSprRelx);
		sprite[nSprite].y 			= y + enumStrGetInt(kPrefabSprRely);
		sprite[nSprite].z 			= z + enumStrGetInt(kPrefabSprRelz);

		sprite[nSprite].picnum  	= (ushort) ClipRange(enumStrGetInt(kPrefabSprPic),  0, kMaxTiles - 1);
		tileLoadTile(sprite[nSprite].picnum);
		if (tilesizx[sprite[nSprite].picnum] <= 0 || tilesizy[sprite[nSprite].picnum] <= 0)
			sprite[nSprite].picnum = 0;

		sprite[nSprite].pal	  		= (BYTE) ClipRange(enumStrGetInt(kPrefabSprPal), 0, 255);
		sprite[nSprite].shade	  	= (schar) ClipRange(enumStrGetInt(kPrefabSprShade), -128, 63);
		sprite[nSprite].xrepeat 	= (BYTE) ClipRange(enumStrGetInt(kPrefabSprXrep), 4, 255);
		sprite[nSprite].yrepeat 	= (BYTE) ClipRange(enumStrGetInt(kPrefabSprYrep), 4, 255);
		sprite[nSprite].xoffset 	= (schar)  ClipRange(enumStrGetInt(kPrefabSprXofs),  -128, 127);
		sprite[nSprite].yoffset 	= (schar)  ClipRange(enumStrGetInt(kPrefabSprYofs),  -128, 127);
		sprite[nSprite].cstat	 	= (short)  enumStrGetInt(kPrefabSprCstat);

		if ((sprite[nSprite].cstat & 48) == 48)
			spriteSetSlope(nSprite, (short) ClipRange(enumStrGetInt(kPrefabSprSlope), -32767, 32767));
		
		sprite[nSprite].ang	  		= (short)  enumStrGetInt(kPrefabSprAng);
		sprite[nSprite].clipdist 	= (BYTE)  ClipRange(enumStrGetInt(kPrefabSprClipdist), 0, 255);
		sprite[nSprite].flags 		= (short)  ClipRange(enumStrGetInt(kPrefabSprHitag), -32767, 32767);
		
		short nType 				= (short)  ClipRange(enumStrGetInt(kPrefabSprLotag), 0, 1023);
		sprite[nSprite].type 		= nType;

		// test if sprite is an xsprite
		if (enumStrGetInt(kPrefabSprRxID, NULL, ',', -1) >= 0) {
			
			// setup xsprite
			nXSprite = GetXSprite(nSprite);
			XSPRITE* pXSpr = &xsprite[nXSprite];
			
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
			
			
			if (!channelsFound) channelsFound = (pXSpr->rxID || pXSpr->txID);
			
		}
		
		if (!pathMarkerFound) pathMarkerFound = (nType == kMarkerPath);
		if (!rorMarkerFound)  rorMarkerFound = (nType > 6 && nType <= 14 && nType != 8);
		
		hgltAdd(OBJ_SPRITE, nSprite);
		i++;

	}
	
	delete pfbFile;
	
	if (i > 0) {
		
		updatenumsprites();
		fspr = pfbAng2FaceSpr(fang);
		fang = (short)(fang + ((fang - 1536) << 1));

		switch (searchstat) {
			case OBJ_WALL:
				if (fspr >= 0) { // set wall angle if correct face side is defined
					hgltSprRotate((fang + (GetWallAngle(searchwall2) + kAng90)) & kAngMask);
					if (width || height) {
						if (wallHoriz(searchwall2)) hgltSprChgXYZ(x - sprite[fspr].x, 0);
						else if (wallVert(searchwall2)) hgltSprChgXYZ(0, y - sprite[fspr].y);
						
						// don't know how to make centerring better for angled walls...:((((
						else hgltSprChgXYZ(x - sprite[fspr].x, y - sprite[fspr].y);
						
					}

				}
				break;
			case OBJ_SPRITE:
				switch (sprite[searchwall].cstat & 48) {
					case kSprWall:
						if (fspr >= 0) { // set wall sprite angle if correct face side is defined
							
							hgltSprRotate((fang + sprite[searchwall].ang) & kAngMask);
							if (width || height)
								hgltSprChgXYZ(x - sprite[fspr].x, y - sprite[fspr].y);
							
						}
						break;
					case kSprFloor:
						z = sprite[searchwall].z;
						hgltSprPutOnZ(z, (nPosZ > z) ? 0x0001 : 0x0002, ofsAboveCeil, ofsBelowFloor);
						break;
					case 48:
						z = spriteGetZOfSlope(searchwall, x, y);
						hgltSprPutOnZ(z, (nPosZ > z) ? 0x0001 : 0x0002, ofsAboveCeil, ofsBelowFloor);
						break;
				}
				break;
			case OBJ_FLOOR:
				hgltSprPutOnFloor(ofsBelowFloor);
				break;
			case OBJ_CEILING:
				hgltSprPutOnCeiling(ofsAboveCeil);
				break;
			
		}
		
		hgltSprCallFunc(sprFixSector);
		AutoAdjustSprites();
		
		if (nXSprite >= 0) {
			
			if (channelsFound)   hgltIsolateChannels(kHgltPoint, kChlCheckOutsideR | kChlCheckInsideRS | kChlCheckOutsideS);
			if (pathMarkerFound) hgltIsolatePathMarkers(kHgltPoint);
			if (rorMarkerFound)  hgltIsolateRorMarkers(kHgltPoint);
			
		}
		
		hgltSprClamp(ofsAboveCeil, ofsBelowFloor);

	}
	
	return i;

}


int pfbDlgFaceAngDefine() {
	
	Window dialog(0, 0, 176, 200, "Define prefab face side");
	
	dialog.Insert(new Label(59, 70, "<PREFAB>"));
	
	dialog.Insert(new BitButton2(66, 88, 34, 34,  pBitmaps[10], mrUser + 1536));
	dialog.Insert(new Label(70, 126, "1536"));
	
	dialog.Insert(new BitButton2(66, 24, 34, 34,  pBitmaps[13], mrUser + 512));
	dialog.Insert(new Label(74, 14, "512"));

	dialog.Insert(new BitButton2(14, 56, 34, 34,  pBitmaps[11], mrUser + 0));
	dialog.Insert(new Label(29, 94, "0"));
	
	dialog.Insert(new BitButton2(122, 56, 34, 34,  pBitmaps[12], mrUser + 1024));
	dialog.Insert(new Label(126, 94, "1024"));

	dialog.Insert(new Label(40, 145, "<YOU ARE HERE>"));
	
	dialog.Insert(new TextButton( 20,  160, 62, 20,  "&None", mrUser + kAng360));
	dialog.Insert(new TextButton( 84,  160, 62, 20,  "&Cancel", mrCancel));
	
	return ShowModal(&dialog);
}

BOOL pfbRemove(char* filename) {
	
	fileAttrSetWrite(filename);
	unlink(filename);
	return (!fileExists(filename));
	
}