/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
// Functions to fix and cleanup objects such as sprites, walls or sectors

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

#include "db.h"
#include "build.h"
#include "xmpstub.h"
#include "aadjust.h"
#include "xmpconf.h"
#include "xmpmisc.h"
#include "tile.h"
#include "tilefav.h"
#include "gameutil.h"
#include "screen.h"
#include "nnexts.h"
#include "hglt.h"
#include "xmptrig.h"
#include "sectorfx.h"
#include "seq.h"
#include "gui.h"
#include "edit3d.h"
#include "mapcmt.h"
#include "xmpror.h"
#include "tracker.h"
#include "xmpstr.h"

BYTE markerDB[6][3] = {
	
	{kSectorRotate 			- kSectorBase, 		kMarkerAxis,				0},
	{kSectorRotateMarked 	- kSectorBase, 		kMarkerAxis,				0},
	{kSectorRotateStep		- kSectorBase, 		kMarkerAxis,				0},
	{kSectorTeleport 		- kSectorBase, 		kMarkerWarpDest,			0},
	{kSectorSlide 			- kSectorBase, 		kMarkerOff, 		kMarkerOn},
	{kSectorSlideMarked		- kSectorBase, 		kMarkerOff, 		kMarkerOn},
	
};

AUTODATA* autoData = NULL;
int autoDataLength = 0;

unsigned short markers[]  = { kMarkerOff, kMarkerOn, kMarkerAxis, kMarkerWarpDest };
unsigned short misc[]     = { kMarkerSPStart, kMarkerMPStart, kMarkerDudeSpawn, kMarkerEarthquake, kGenSound, kSoundSector, kSoundPlayer };
unsigned short cond[] 	  = { kModernCondition, kModernConditionFalse };

SYS_STATNUM_GROUP sysStatData[] =
{
	{ 0x02,		kStatMarker,					0,							0, 					markers, LENGTH(markers) },
	{ 0x03,		kStatPathMarker,				kMarkerPath,				0,					NULL, 0 },
	{ 0x03,		kStatDude,						kDudeBase, 					kDudeMax - 1,		NULL, 0 },
	{ 0x03,		kStatThing,						kThingBase, 				kThingMax - 1,		NULL, 0 },
	{ 0x03,		kStatItem,						kItemWeaponBase, 			kItemMax - 1,		NULL, 0 },
	{ 0x03,		kStatProjectile,				kMissileBase, 				kMissileMax - 1,	NULL, 0 },
	{ 0x03,		kStatTraps,						kTrapBase, 					kTrapMax - 1,		NULL, 0 },
	{ 0x03,		kStatAmbience,					kSoundAmbient,				0,					NULL, 0 },
	{ 0x03,		kStatDecoration,				0,							0, 					misc, LENGTH(misc) },
	{ 0x03,		kStatModernStealthRegion,		kModernStealthRegion,		0,					NULL, 0 },
	{ 0x03,		kStatModernDudeTargetChanger,	kModernDudeTargetChanger,	0,					NULL, 0 },
	{ 0x03,		kStatModernCondition,			0,							0, 					cond, LENGTH(cond) },
};

char isSysStatnum(int nStat)
{
	int i;
	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		SYS_STATNUM_GROUP* data = &sysStatData[i];
		if (nStat == data->statnum)
			return true;
	}
	
	return false;
}


enum
{
	kAutoAdjustParGroup,
	kAutoAdjustParSeq,
	kAutoAdjustParTile,
	kAutoAdjustParSizeX,
	kAutoAdjustParSizeY,
	kAutoAdustParPal,
	kAutoAdjustParHit,
	kAutoAdjustParFlags,
	
};
static NAMED_TYPE gNTAutoAdjustParams[] =
{
	{kAutoAdjustParGroup,		"Group"},
	{kAutoAdjustParSeq,			"Seq"},
	{kAutoAdjustParTile,		"Tile"},
	{kAutoAdjustParSizeX,		"SizeX"},
	{kAutoAdjustParSizeY,		"SizeY"},
	{kAutoAdustParPal,			"Pal"},
	{kAutoAdjustParHit,			"Hit"},
	{kAutoAdjustParFlags,		"Flags"},
};

enum
{
	kAutoAdjustParFlagsException	= 0x01,
	kAutoAdjustParFlagsXSprite		= 0x02,
};
static NAMED_TYPE gAutoAdjustFlags[] =
{
	{kAutoAdjustParFlagsException,	"Exception"},
	{kAutoAdjustParFlagsXSprite,	"XSprite"},
};

void AutoAdjustSpritesInit(void)
{
	char key[256], val[256], *pKey, *pVal;
	int nPar, nType, nVal, nPrevNode = -1, o;
	short pic, xr, yr, plu;
	RESHANDLE hIni;
	
	AUTODATA model, tmp;
	model.group		= kOGrpNone;	model.exception		= 0;
	model.xsprite	= 0;			model.type			= -1;
	model.seq		= -1;			model.picnum		= -1;
	model.xrepeat	= -1;			model.yrepeat		= -1;
	model.hitBit	= -1;			model.plu			= kPluAny;
	
	if (autoData)
		free(autoData);
	
	autoDataLength = 0; autoData = NULL;
	if ((hIni = gGuiRes.Lookup((unsigned int)4, "INI")) == NULL)
		return;
	
	IniFile* pIni = new IniFile((BYTE*)gGuiRes.Load(hIni), hIni->size);
	while(pIni->GetNextString(NULL, &pKey, &pVal, &nPrevNode, "AutoAdjustSprites"))
	{
		if (isIdKeyword(pKey, "Type", &nType) && rngok(nType, 0, 1024))
		{
			o = 0;
			memcpy(&tmp, &model, sizeof(AUTODATA));
			while((o = enumStr(o, pVal, key, val)) > 0)
			{
				switch(nPar = findNamedID(key, gNTAutoAdjustParams, LENGTH(gNTAutoAdjustParams)))
				{
					case kAutoAdjustParGroup:
						tmp.group = words2flags(val, gGameObjectGroupNames, LENGTH(gGameObjectGroupNames));
						break;
					case kAutoAdjustParFlags:
						nVal = words2flags(val, gAutoAdjustFlags, LENGTH(gAutoAdjustFlags));
						tmp.exception	= ((nVal & kAutoAdjustParFlagsException) > 0);
						tmp.xsprite		= ((nVal & kAutoAdjustParFlagsXSprite) > 0);
						break;
					default:
						if (isfix(val) && (nVal = atoi(val)) >= -1)
						{
							switch(nPar)
							{
								case kAutoAdjustParTile:	if (nVal < kMaxTiles)	tmp.picnum = nVal;		break;
								case kAutoAdjustParSeq:		if (nVal < 65536)		tmp.seq = nVal;			break;
								case kAutoAdjustParSizeX:	if (nVal < 256)			tmp.xrepeat = nVal;		break;
								case kAutoAdjustParSizeY:	if (nVal < 256)			tmp.yrepeat = nVal;		break;
								case kAutoAdustParPal:		if (nVal < 256)			tmp.plu = nVal;			break;
								case kAutoAdjustParHit:		if (nVal < 2)			tmp.hitBit = nVal;		break;
							}
						}
						break;
				}
			}
			
			// now check if it's worthy
			if (memcmp(&tmp, &model, sizeof(AUTODATA)) != 0)
			{
				autoData = (AUTODATA*)realloc(autoData, sizeof(AUTODATA) * (autoDataLength + 1));
				dassert(autoData != NULL);
				tmp.type = nType;
				
				memcpy(&autoData[autoDataLength], &tmp, sizeof(AUTODATA));
				autoDataLength++;
			}
		}
	}
	
	delete(pIni);
	
	for (o = 0; o < autoDataLength; o++)
	{
		AUTODATA* pData = &autoData[o];
		// override appearance by info from first frame of seq animation
		if (pData->seq >= 0 && getSeqPrefs(pData->seq, &pic, &xr, &yr, &plu))
		{
			pData->picnum  = pic;
			pData->xrepeat = xr;
			pData->yrepeat = yr;
			pData->plu     = plu;
			pData->xsprite = 1; // if it have seq, then it must be xsprite
		}
	}
}

void AutoAdjustSprites(void)
{
	if (gPreviewMode || !gAutoAdjust.enabled)
		return;

	short nSector, nSprite;
	int j = 0, k = 0, i = 0, zTop, zBot;
	spritetype* pSprite = NULL; XSPRITE* pXSprite = NULL;

	for (nSector = 0; nSector < numsectors; nSector++)
	{
		for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			pSprite = &sprite[nSprite];
			if (pSprite->statnum == kStatFree)
				continue;

			int iPicnum = -1, iType = -1, nXSprite = -1, otype = pSprite->type;
			
			CleanUpStatnum(nSprite);

			i = -1;
			iPicnum = adjIdxByTileInfo(pSprite->picnum, 0); 	// search by pic
			iType 	= adjIdxByType(pSprite->type); 				// search by type

			// this set of rules determine which row to use from the autoData table
			if (iPicnum >= 0)
			{
				// if we found by picnum, but picnum of otype == -1 in autodata, give it priority
				if ((i = adjIdxByType(otype)) >= 0)
					i = (autoData[i].picnum < 0) ? -2 : iPicnum;
			}

			if (i > -2)
			{
				if (iType >= 0) i = iType;
				if (iPicnum >= 0 && autoData[iPicnum].type == pSprite->type)
					i = iPicnum;
			}

			if (i < 0)
				continue; // if nothing found, ignore this sprite

			if (autoData[i].xsprite)
			{
				nXSprite = GetXSprite(nSprite);
				pXSprite = &xsprite[nXSprite];
			}

			short type = autoData[i].type;
			if (!autoData[i].exception) {

				pSprite->type = type;
				adjSetApperance(pSprite, i);

				if (pSprite->extra > 0)
				{
					nXSprite = pSprite->extra;
					pXSprite = &xsprite[pSprite->extra];
				}

				switch (type)
				{
					case 16:
						pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
						pSprite->cstat |= kSprInvisible;
						pSprite->shade = -128;
						if (pXSprite->data2 > 0 || pXSprite->data3 > 0) pSprite->pal = kPlu1;
						else pSprite->pal = kPlu10;
						break;
					case kTrapExploder:
						pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
						pSprite->cstat |= kSprInvisible;
						pSprite->shade = -128;
						break;
					case kDudeModernCustom:
					case kModernCustomDudeSpawn:
						cdGetSeq(pSprite);
						break;
					case kDudeCerberusOneHead:
						if (pXSprite->rxID || pXSprite->triggerProximity || pXSprite->triggerTouch || pXSprite->triggerSight) break;
						pXSprite->rxID = 7;
						break;
					case kMarkerSPStart:
					case kMarkerMPStart:
						pXSprite->data1 &= 7;
						pSprite->picnum = (short)(2522 + pXSprite->data1);
						if (!gModernMap || pSprite->type != kMarkerMPStart) break;
						else if (pXSprite->data2 > 0)
						{
							pSprite->pal = (pXSprite->data2 == 1) ? kPlu10 : kPlu2;
						}
						break;
					case kMarkerUpLink:
					case kMarkerUpWater:
					case kMarkerUpGoo:
					case kMarkerUpStack:
					case kMarkerLowLink:
					case kMarkerLowWater:
					case kMarkerLowGoo:
					case kMarkerLowStack:
					case kMarkerDudeSpawn:
					case kMarkerEarthquake:
						pSprite->cstat &= ~kSprFlipY;
						if (!gModernMap) break;
						else if (type == kMarkerDudeSpawn)
						{
							for (k = 0, j = 0; k < 4; k++)
							{
								if ((j+= getDataOf((BYTE)k, OBJ_SPRITE, nSprite)) >= kDudeMax) {
									pSprite->picnum = 821;
									break;
								}
							}
						}
						break;
					case kDudeZombieAxeBuried:
						// place earth zombies to the ground
						if (pSprite->sectnum < 0) break;
						GetSpriteExtents(pSprite, &zTop, &zBot);
						pSprite->z = sector[pSprite->sectnum].floorz + getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot;
						break;
					case kThingFlammableTree:
						// try to find correct picnum
						switch (pXSprite->data1) {
							case 1:  pSprite->picnum = 547; break;
							case 0:  pSprite->picnum = 542; break;
							default:
								switch (pSprite->picnum) {
									case 3363:
										pXSprite->data1 = 1;
										pSprite->picnum = 547;
										break;
									case 3355:
										pXSprite->data1 = 0;
										pSprite->picnum = 542;
										break;
								}
								break;
						}
						break;
					case kSoundAmbient:
						if (pXSprite->data1 && pXSprite->data2 && pXSprite->data1 >= pXSprite->data2)
						{
							int tmp = pXSprite->data2;
							pXSprite->data2 = pXSprite->data1;
							pXSprite->data1 = tmp;

							if (pXSprite->data2 == pXSprite->data1)
								pXSprite->data2 = pXSprite->data1 << 1;
						}
						break;
				}
			}

			type = pSprite->type;
			if (autoData[i].group == kOGrpMarker)
			{
				pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
				pSprite->cstat |= kSprInvisible;
				pSprite->shade = -128;
			}
			else if (rngok(type, kSwitchBase, kSwitchMax)
				|| rngok(type, kItemWeaponBase, kItemMax) || rngok(type, kDudeBase, kDudeMax))
					pSprite->cstat &= ~kSprBlock;

			// make sure sprites aren't in the floor or ceiling
			if (pSprite->statnum == kStatDude)
				clampSprite(pSprite);
		}
	}
}

void adjSetApperance(spritetype* pSprite, int idx) {

	if (!gAutoAdjust.enabled)
		return;

	int faved = tileInFaves(pSprite->picnum);
	if (faved < 0 || gFavTiles[faved].pic <= 0)
	{
		if (gAutoAdjust.setPic && autoData[idx].picnum >= 0)
			pSprite->picnum = autoData[idx].picnum;

		if (gAutoAdjust.setPlu && autoData[idx].plu >= 0)
			pSprite->pal = (BYTE)autoData[idx].plu;

		if (gAutoAdjust.setSize)
		{
			if (autoData[idx].xrepeat >= 0) pSprite->xrepeat = (BYTE)autoData[idx].xrepeat;
			if (autoData[idx].yrepeat >= 0) pSprite->yrepeat = (BYTE)autoData[idx].yrepeat;
		}

		if (pSprite->type != autoData[idx].type && autoData[idx].plu == kPluAny)
			pSprite->pal = kPlu0;
	}

	if (gAutoAdjust.setHitscan)
	{
		if (autoData[idx].hitBit == 0)     pSprite->cstat &= ~kSprHitscan;
		else if (autoData[idx].hitBit > 0) pSprite->cstat |= kSprHitscan;
	}

	switch (pSprite->type) {
		case kDudeModernCustom:
			cdGetSeq(pSprite);
			break;
		case kThingFlammableTree:
			return;
	}

}

int adjIdxByType(int nType) {

	int i;
	for (i = 0; i < autoDataLength; i++)
	{
		if (nType != autoData[i].type) continue;
		return i;
	}

	return -1;
}

BOOL adjSpriteByType(spritetype* pSprite) {

	int i;
	if ((i = adjIdxByType(pSprite->type)) < 0)
		return FALSE;

	adjSetApperance(pSprite, i);
	clampSprite(pSprite);
	return TRUE;
}

// how many of *same* pictures should be skipped before we meet required?
int adjCountSkips(int pic) {

	int skip = 0;
	for (int i = 0; i < tileIndexCount; i++)
	{
		if (i == tileIndexCursor) break;
		else if (tileIndex[i] == pic)
			skip++;
	}

	return skip;

}

// returns required index of AUTODATA array
int adjIdxByTileInfo(int pic, int skip) {

	int i = 0, j = 0, k = 0;
	for (i = 0; i < autoDataLength; i++)
	{
		if (autoData[i].picnum == pic && k++ == skip)
			return i;
	}

	return -1;

}

int adjFillTilesArray(int objectGroup)
{
	int i = 0, j = 0;
	for (j = 0, tileIndexCount = 0; j < autoDataLength; j++)
	{
		if (autoData[j].picnum < 0 || !(autoData[j].group & objectGroup)) continue;
		if (!gSpriteNames[autoData[j].type])
			continue;
		
		tileIndex[tileIndexCount] 	 = (short)autoData[j].picnum;
		tilePluIndex[tileIndexCount] = (char)ClipRange(autoData[j].plu, 0, kPluMax);
		tileIndexCount++;
	}

	return tileIndexCount;
}

int FixMarker(int nSect, int nMrk, int nMrkType)
{
	register int t;
	if (!rngok(nMrk, 0, kMaxSprites) || sprite[nMrk].statnum != kStatMarker || sprite[nMrk].type != nMrkType)
		nMrk = -1;
	
	if (nMrk >= 0)
	{
		if (sprite[nMrk].owner != nSect)
		{
			t = InsertSprite(sprite[nMrk].sectnum, kStatMarker);
			sprite[t] = sprite[nMrk];
			sprite[t].index = t;
			sprite[t].owner = nSect;
			nMrk = t;
		}
	}
	else if (nMrkType)
	{
		nMrk = InsertSprite(nSect, kStatMarker);
		sprite[nMrk].x = wall[sector[nSect].wallptr].x;
		sprite[nMrk].y = wall[sector[nSect].wallptr].y;
		sprite[nMrk].owner = nSect;
		sprite[nMrk].type = nMrkType;
	}
	
	return nMrk;
}

void CleanUp() {

	int  i, j;
	XSECTOR* pXSect; sectortype* pSect;
		
	if (gPreviewMode)
		return;
	
	dbXSectorClean();
	dbXWallClean();
	dbXSpriteClean();
	
	for (i = 0; i < numsectors; i++)
	{
		pSect = &sector[i];
		if (pSect->extra > 0)
		{
			j = 0;
			pXSect =&xsector[pSect->extra];
			while(j < LENGTH(markerDB))
			{
				BYTE* pDB = markerDB[j];
				if (pSect->type - kSectorBase == pDB[0])
				{
					pXSect->marker0 = (pDB[1]) ? FixMarker(i, pXSect->marker0, pDB[1]) : -1;
					pXSect->marker1 = (pDB[2]) ? FixMarker(i, pXSect->marker1, pDB[2]) : -1;
					break;
				}
				
				j++;
			}
			
			if (j >= LENGTH(markerDB))
				pXSect->marker0 = pXSect->marker1 = -1;
		}
	}
		
	for (i = headspritestat[kStatMarker]; i != -1; i = j)
	{
		j = nextspritestat[i];
		if (sprite[i].extra > 0)
			dbDeleteXSprite(sprite[i].extra);

		sprite[i].cstat |= kSprInvisible;
		sprite[i].cstat &= ~(kSprBlock | kSprHitscan);

		int nSector = sprite[i].owner;
		int nXSector = sector[nSector].extra;

		if (rngok(nSector, 0, numsectors) && rngok(nXSector, 1, kMaxXSectors))
		{
			switch (sprite[i].type)
			{
				case kMarkerOff:
				case kMarkerAxis:
				case kMarkerWarpDest:
					sprite[i].picnum = 3997;
					if (xsector[nXSector].marker0 == i)
						continue;
					break;
				case kMarkerOn:
					sprite[i].picnum = 3997;
					if (xsector[nXSector].marker1 == i)
						continue;
					break;
			}
		}

		DeleteSprite(i);
	}
	
	spritesortcnt = 0;
	
	warpInit();
	InitSectorFX();
	gCommentMgr.Cleanup();
	CXTracker::Cleanup();
	MapStats::Collect();
	
	if (gMisc.pan)
		AlignSlopes();
}

void CleanUpStatnum(int nSpr) {

	if (gPreviewMode)
		return;
	
	int i, j;
	spritetype* pSpr =& sprite[nSpr];

	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		int chgTo = -1;
		SYS_STATNUM_GROUP* data = &sysStatData[i];
		
		// set proper statnum for types in a range
		if ((data->check & 0x01) && pSpr->statnum != data->statnum)
		{
			if (data->enumArray)
			{
				for (j = 0; j < data->enumLen; j++)
				{
					if (pSpr->type != data->enumArray[j]) continue;
					chgTo = data->statnum;
					break;
				}
			}
			else if (data->typeMax <= 0 && pSpr->type == data->typeMin) chgTo = data->statnum;
			else if (data->typeMax  > 0 && irngok(pSpr->type, data->typeMin, data->typeMax))
				chgTo = data->statnum;
		}

		// erase statnum for types that are NOT in a range
		if ((data->check & 0x02) && pSpr->statnum == data->statnum)
		{
			if (data->enumArray)
			{
				for (j = 0; j < data->enumLen; j++)
				{
					if (pSpr->type == data->enumArray[j])
						break;
				}
				
				if (j >= data->enumLen)
					chgTo = 0;
			}
			else if (data->typeMax <= 0 && pSpr->type != data->typeMin) chgTo = 0;
			else if (data->typeMax  > 0 && !irngok(pSpr->type, data->typeMin, data->typeMax))
				chgTo = 0;
		}
		
		if (chgTo >= 0)
		{
			if (!gAutoAdjust.setStatnumThings && rngok(pSpr->type, kThingBase, kThingMax))
			{
				if (!isSysStatnum(pSpr->statnum))
					break;
			}
						
			if (data->check & 0x04) pSpr->statnum = chgTo;
			else ChangeSpriteStat(nSpr, chgTo);
		}
	}
}

void CleanUpMisc() {

	if (gPreviewMode)
		return;
	
	int i, j, s, e;
	for (i = 0; i < numsectors; i++)
	{
		sectortype* pSector = &sector[i];

		// useless x-object check
		if (pSector->extra > 0)
		{
			if (pSector->type == 0 && obsoleteXObject(OBJ_FLOOR, pSector->extra))
				dbDeleteXSector(pSector->extra);
			else
			{
				XSECTOR* pXSect = &xsector[pSector->extra];
				if(pXSect->txID && !pXSect->triggerOn && !pXSect->triggerOff)
					pXSect->triggerOn = pXSect->triggerOff = 1;
			}
		}

		// fix ceiling slopes
		if (pSector->ceilingslope != 0)
		{
			if (!(pSector->ceilingstat & kSectSloped))
				pSector->ceilingslope = 0;
		}
		else if (pSector->ceilingstat & kSectSloped)
		{
			pSector->ceilingstat &= ~kSectSloped;
		}

		// fix floor slopes
		if (pSector->floorslope != 0)
		{
			if (!(pSector->floorstat & kSectSloped))
				pSector->floorslope = 0;
		}
		else if (pSector->floorstat & kSectSloped)
		{
			pSector->floorstat &= ~kSectSloped;
		}
		
		getSectorWalls(i, &s, &e);
		for (j = s; j <= e; j++)
		{
			walltype* pWall = &wall[j];
			
			// useless x-object check
			if (pWall->extra > 0)
			{
				if (pWall->type == 0 && obsoleteXObject(OBJ_WALL, pWall->extra))
					dbDeleteXWall(pWall->extra);
				else
				{
					XWALL* pXWall =& xwall[pWall->extra];
					if(pXWall->txID && !pXWall->triggerOn && !pXWall->triggerOff)
						pXWall->triggerOn = pXWall->triggerOff = 1;
				}
			}
			
			if (pWall->nextwall >= 0)
				continue;

			// clear useless cstat for white walls...
			if (pWall->cstat & kWallMasked) pWall->cstat &= ~kWallMasked;
			if (pWall->cstat & kWallBlock) pWall->cstat &= ~kWallBlock;
			if (pWall->cstat & kWallHitscan) pWall->cstat &= ~kWallHitscan;
			if (pWall->cstat & kWallOneWay) pWall->cstat &= ~kWallOneWay;
			if (pWall->cstat & kWallSwap) pWall->cstat &= ~kWallSwap;
			if (pWall->cstat & kWallTransluc) pWall->cstat &= ~kWallTransluc;
			if (pWall->cstat & kWallTranslucR) pWall->cstat &= ~kWallTranslucR;
		}
		
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			spritetype* pSprite =& sprite[j];

			// useless x-sprites check
			if (pSprite->statnum == kMaxStatus) continue;
			else if (!pSprite->type && pSprite->extra > 0 && obsoleteXObject(OBJ_SPRITE, pSprite->extra))
				dbDeleteXSprite(pSprite->extra);
			
			CleanUpStatnum(j);
			
			if (pSprite->extra > 0)
			{
				XSPRITE* pXSpr =&xsprite[sprite[j].extra];

				// no pickup for non-item
				if (pSprite->statnum != kStatItem && pXSpr->triggerPickup)
					pXSpr->triggerPickup = FALSE;
				
				// item out of a range
				if (pXSpr->dropItem >= kItemMax)
					pXSpr->dropItem = 0;
				
				if(pXSpr->txID && !pXSpr->triggerOn && !pXSpr->triggerOff)
					pXSpr->triggerOn = pXSpr->triggerOff = 1;
				
				if (pSprite->statnum == kStatDude)
				{
					// use KEY instead of DROP field for key drop
					if (pXSpr->dropItem >= kItemKeySkull && pXSpr->dropItem <= kItemKey7)
						pXSpr->key = 1 + pXSpr->dropItem - kItemKeySkull, pXSpr->dropItem = 0;
				}
			}
		}
	}
}

/* BOOL sysStatReserved(int nStat) {
	
	int i; 
	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		if (nStat == sysStatData[i].statnum) return TRUE;
	}
	
	return FALSE;
}

BOOL sysStatCanChange(spritetype* pSpr) {
	
	int i, j;
	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		SYS_STATNUM_GROUP* data = &sysStatData[i];
		if (data->enumArray)
		{
			for (j = 0; j < data->enumLen; j++)
			{
				if (pSpr->type == data->enumArray[j])
					return FALSE;
			}
		}
		else if (data->typeMax <= 0 && pSpr->type == data->typeMin) return FALSE;
		else if (irngok(pSpr->type, data->typeMin, data->typeMax))  return FALSE;
	}
	
	return TRUE;
} */
