/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Favorite tiles manager.
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
#include "build.h"
#include "baselayer.h"
#include "gameutil.h"
#include "screen.h"
#include "tile.h"
#include "xmpstub.h"
#include "common_game.h"
#include "inifile.h"
#include "aadjust.h"
#include "xmpmisc.h"
#include "tilefav.h"
#include "enumstr.h"
#include "gui.h"

short gFavTilesC = 0;
GAME_OBJECT_TILE gFavTiles[];
IniFile FavoriteTilesIni(kFavTilesFileName);

void favoriteTileInit() {

	char tmp[255];
	int j, nType, nPic;
	ININODE* prev = NULL; char* key = NULL; char* value;
	memset(gFavTiles, -1, sizeof(gFavTiles)); gFavTilesC = 0;
	while (FavoriteTilesIni.GetNextString(tmp, &key, &value, &prev, "Favorites"))
	{
		if ((nPic = enumStrGetInt(0, value)) <= 0 || nPic >= kMaxTiles) continue;
		else if ((nType = enumStrGetInt(1, NULL)) < 0 || nType >= 1024) continue;
		else if ((j = tileInFaves(nPic)) >= 0) // already in faves? replace it
		{
			gFavTiles[j].pic  = (short) nPic;
			gFavTiles[j].type = (short) nType;
		}
		else
		{
			gFavTiles[gFavTilesC].pic  = (short) nPic;
			gFavTiles[gFavTilesC].type = (short) nType;
			gFavTilesC++;
		}
	}
}
void favoriteTileSave() {

	if (FavoriteTilesIni.SectionExists("Favorites"))
		FavoriteTilesIni.RemoveSection("Favorites");
	
	char key[4]; char value[32];
	if (gFavTilesC > 0) {
		for (int i = 0, j = 0; i < gFavTilesC; i++) {
			if (gFavTiles[i].pic == -1 || gFavTiles[i].type == -1)
				continue;

			sprintf(key, "%d", j++);
			sprintf(value, "%d,%d", gFavTiles[i].pic, gFavTiles[i].type);
			FavoriteTilesIni.PutKeyString("Favorites", key, value);

		}
	}
	
	FavoriteTilesIni.Save();
	return;

}

int favoriteTileAddSimple(short type, short picnum) {
	
	short nType = 0; int i = 0;
	
	// try to offer type from autoData (search by pic)
	for (i = 0; i < autoDataLength; i++) {
		if (autoData[i].picnum != picnum) continue;
		nType = (short) ClipLow(autoData[i].type, 0);
		break;

	}
	
	memset(buffer, 0, LENGTH(buffer));
	nType = (short)ClipRange((nType <= 0) ? type : nType, 0, 1023);
	if (gSpriteNames[nType])
		sprintf(buffer, gSpriteNames[nType]);

	if ((nType = name2TypeId(buffer)) >= 0) {
		
		picnum = (short) picnum;
		nType  = (short) nType;

		// check if this pic already in faves
		if ((i = tileInFaves(picnum)) >= 0) {
			
			if (gFavTiles[i].type != nType) {
				if (!Confirm("Change type from %d to %d?", gFavTiles[i].type, nType))
					return -1;
			}
			
			gFavTiles[i].type = nType;
			return i;
			
		}
		
		gFavTiles[gFavTilesC].pic  = (short) picnum;
		gFavTiles[gFavTilesC].type = nType;
		return gFavTilesC++;
		
	}

	return -1;
	
}

BOOL favoriteTileRemove(int nTile) {

	int i = 0, j = 0;
	for (i = 0; i < gFavTilesC; i++) {
		if (gFavTiles[i].pic != nTile) continue;
		gFavTiles[i].pic = gFavTiles[i].type = -1;
		break;
	}
	
	if (i == gFavTilesC)
		return FALSE;
	
	// adjust array
	if (gFavTilesC - 1 > 0) {
		
		for (j = i + 1; j < gFavTilesC; j++, i++) {
			gFavTiles[i].pic  = gFavTiles[j].pic;
			gFavTiles[i].type = gFavTiles[j].type;

			gFavTiles[j].pic = gFavTiles[j].type = -1;
		}
		
	} else {
		
		gFavTiles[0].pic = gFavTiles[0].type = -1;
		
	}
	
	gFavTilesC--;
	return TRUE;
	
}

int favoriteTileSelect(int startPic, int nDefault, BOOL returnTile, int objType) {

	BOOL startPicFound = FALSE;
	int i = 0, j = 0; int picnum = -1, idx = -1;
	
	if (gFavTilesC > 0) {
		
		// fill tile index array
		for (i = 0, tileIndexCount = 0; i < gFavTilesC; i++) {
			if (gFavTiles[i].pic == -1 || gFavTiles[i].type == -1)
				continue;
			
			tileIndex[tileIndexCount] = gFavTiles[i].pic;

			// get predefined PLU from autoData 
			switch (objType) {
				case OBJ_SPRITE:
				case OBJ_FLATSPRITE:
					for (int j = 0; j < autoDataLength; j++) {
						if (autoData[j].type != gFavTiles[i].type || autoData[j].picnum != gFavTiles[i].pic) continue;
						tilePluIndex[tileIndexCount] = (char) ClipRange(autoData[j].plu, 0, kPluMax);
						break;
					}
					break;
			}
			
			if (gFavTiles[i].pic == startPic)
				startPicFound = TRUE;
			
			tileIndexCount++;

		}

		if ((picnum = tilePick((startPicFound) ? startPic : -1, -1, OBJ_CUSTOM, "Favorite tiles")) >= 0) {

			for (i = 0; i < gFavTilesC; i++) {
				if (gFavTiles[i].pic != picnum) continue;
				else idx = i;
				break;
			}
			
		}
		
	}
	
	return (idx < 0) ? nDefault : (returnTile) ? picnum : idx;
}

spritetype* favTileInsert(int where, int nSector, int x, int y, int z, int nAngle) {

	spritetype* pSprite = NULL; int idx = favoriteTileSelect(-1, -1, FALSE, OBJ_SPRITE);
	if (idx < 0) return pSprite; 

	int nSprite = InsertSprite(nSector, kStatDecoration);
	updatenumsprites();
	
	pSprite = &sprite[nSprite];
	
	// set type according selected picnum
	pSprite->picnum = gFavTiles[idx].pic;
	pSprite->type = gFavTiles[idx].type;
	
	AutoAdjustSprites();
	
	pSprite->x = x; pSprite->y = y; pSprite->z = z;
	pSprite->ang = (short)nAngle;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == OBJ_FLOOR) pSprite->z += getflorzofslope(nSector, pSprite->x, pSprite->y) - zBot;
	else pSprite->z += getceilzofslope(nSector, pSprite->x, pSprite->y) - zTop;

	return pSprite;
		
}

int tileInFaves(int picnum) {
	
	for (int i = 0; i < gFavTilesC; i++) {
		if (gFavTiles[i].pic == picnum)
			return i;
	}
	
	return -1;
	
}
