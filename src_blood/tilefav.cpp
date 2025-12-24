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

#include "tile.h"
#include "xmpmaped.h"
#include "aadjust.h"

static FAVTILE item = {-1, -1};
VOIDLIST gFavTiles(sizeof(item), &item);

void favoriteTileInit()
{
    IniFile* pIni = new IniFile(kFavTilesFileName);
    int nType, nPic, nPrevNode = -1;
    char *key, *value;
    
    gFavTiles.Clear();
    while (pIni->GetNextString(&key, &value, &nPrevNode, "Favorites"))
    {
        if ((nPic = enumStrGetInt(0, value)) <= 0 || nPic >= kMaxTiles) continue;
        else if ((nType = enumStrGetInt(1, NULL)) < 0 || nType >= 1024) continue;
        item.pic  = (short)nPic, item.type = (short)nType;
        gFavTiles.AddUnique(&item);
    }
    
    delete(pIni);
}

void favoriteTileSave()
{
    IniFile* pIni; FAVTILE* p;
    char key[4], value[32];
    int i = 0;
    
    if (fileExists(kFavTilesFileName))
        fileDelete(kFavTilesFileName);
    
    if (gFavTiles.Length() > 0)
    {
        pIni = new IniFile(kFavTilesFileName);
        
        for (p = (FAVTILE*)gFavTiles.First(), i = 0; p->pic >= 0; p++, i++)
        {
            sprintf(key, "%d", i);
            sprintf(value, "%d,%d", p->pic, p->type);
            pIni->PutKeyString("Favorites", key, value);
        }
        
        pIni->Save();
        delete(pIni);
    }
}

char favoriteTileAddSimple(short type, short picnum)
{
    FAVTILE *p;
    char name[64] = "\0";
    int i;
    
    item.type = 0;
    item.pic  = picnum;
    
    // try to offer type from autoData (search by pic)
    for (i = 0; i < autoDataLength; i++)
    {
        if (autoData[i].picnum != picnum) continue;
        item.type = ClipLow(autoData[i].type, 0);
        break;
    }
    
    item.type = ClipRange((item.type <= 0) ? type : item.type, 0, 1023);
    if (gSpriteNames[item.type])
        strcpy(name, gSpriteNames[item.type]);

    if ((item.type = name2TypeId(name)) >= 0)
    {
        // check if this pic already in faves
        if ((p = tileInFaves(picnum)) != NULL)
        {
            if (p->type != item.type && !Confirm("Change type from %d to %d?", p->type, item.type))
                return 0;
            
            p->type = item.type;
            return 1;
        }
        
        gFavTiles.Add(&item);
        return 1;
    }

    return 0;

}

char favoriteTileRemove(int nTile)
{
    for (FAVTILE* p = (FAVTILE*)gFavTiles.First(); p->pic >= 0; p++)
    {
        if (p->pic != nTile)
            continue;
        
        gFavTiles.Remove(p);
        return 1;
    }
    
    return 0;
}

int favoriteTileSelect(int startPic, int nDefault, int objType)
{
    int i, j; int nTile = -1;
    char startPicFound = 0;
    FAVTILE* p;
    
    tileIndexCount = 0;
    
    for (p = (FAVTILE*)gFavTiles.First(); p->pic >= 0; p++)
    {
        tileIndex[tileIndexCount] = p->pic;
        
        // get predefined PLU from autoData
        switch (objType)
        {
            case OBJ_SPRITE:
            case OBJ_FLATSPRITE:
                for (i = 0; i < autoDataLength; i++)
                {
                    if (autoData[i].type != p->type || autoData[i].picnum != p->pic) continue;
                    tilePluIndex[tileIndexCount] = (char)ClipRange(autoData[i].plu, 0, kPluMax);
                    break;
                }
                break;
        }
        
        tileIndexCount++;
        if (p->pic == startPic)
            startPicFound = 1;
    }
    
    if (tileIndexCount > 0 &&
        (nTile = tilePick((startPicFound) ? startPic : -1, -1, OBJ_CUSTOM, "Favorite tiles")) >= 0)
            return nTile;
    
    return nDefault;
}

spritetype* favTileInsert(int where, int nSector, int x, int y, int z, int nAngle)
{
    int nSpr, nTile;
    spritetype* pSpr;
    
    if ((nTile = favoriteTileSelect(-1, -1, OBJ_SPRITE)) < 0
        || (nSpr = InsertSprite(nSector, kStatDecoration)) < 0)
            return NULL;
    
    updatenumsprites();

    pSpr = &sprite[nSpr];
    pSpr->picnum = nTile;
    
    for (FAVTILE* p = (FAVTILE*)gFavTiles.First(); p->pic >= 0; p++)
    {
        if (p->pic != nTile) continue;
        pSpr->type = p->type; // set type according selected picnum
        break;
    }
    
    AutoAdjustSprites();

    pSpr->x = x; pSpr->y = y; pSpr->z = z;
    pSpr->ang = (short)nAngle;
    
    clampSprite(pSpr);
    return pSpr;

}

FAVTILE* tileInFaves(int nTile)
{
    for (FAVTILE* p = (FAVTILE*)gFavTiles.First(); p->pic >= 0; p++)
    {
        if (p->pic == nTile)
            return p;
    }

    return NULL;
}