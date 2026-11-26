/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026: Originally written by by NoOne.
// Callbacks for various batch map processing
// functions.
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
#include "db.h"
#include "xmpmaped.h"
#include "xmpsky.h"


////////////////////////////////////////////////////
/***********************************************/
/** SPRITE FUNCTIONS                          **/
/***********************************************/
//////////////////


char sprPutOnCeil(spritetype *pSpr, int nOffs)
{
    int zt, zb;
    GetSpriteExtents(pSpr, &zt, &zb);
    if ((pSpr->cstat & kSprRelMask) == kSprSloped) // sloped sprites on sloped sectors...
    {
        if ((sector[pSpr->sectnum].ceilingstat & kSectSloped)
            && sector[pSpr->sectnum].ceilingslope != 0)
                zt = pSpr->z;
    }

    pSpr->z += getceilzofslope(pSpr->sectnum, pSpr->x, pSpr->y) - zt;
    pSpr->z += nOffs;
    return 1;
}

char sprPutOnFloor(spritetype *pSpr, int nOffs)
{
    int zt, zb;
    GetSpriteExtents(pSpr, &zt, &zb);
    if ((pSpr->cstat & kSprRelMask) == kSprSloped) // sloped sprites on sloped sectors...
    {
        if ((sector[pSpr->sectnum].floorstat & kSectSloped)
            && sector[pSpr->sectnum].floorslope != 0)
                zb = pSpr->z;
    }

    pSpr->z += getflorzofslope(pSpr->sectnum, pSpr->x, pSpr->y) - zb;
    pSpr->z += nOffs;
    return 1;
}

char sprZChg(spritetype *pSpr, int nVal)
{
    pSpr->z += nVal;
    return 1;
}

char sprZChgNext(spritetype *pSpr, int nVal)
{
    int v = klabs(nVal);
    pSpr->z = (nVal < 0) ? DecNext(pSpr->z, v) : IncNext(pSpr->z, v);
    return 1;
}

char sprCstatAdd(spritetype* pSpr, int nVal)
{
    nVal = (short)nVal;
    if ((pSpr->cstat & nVal) != 0)
        return 0;
    
    pSpr->cstat |= ((short)nVal);
    return 1;
}

char sprCstatRem(spritetype* pSpr, int nVal)
{
    nVal = (short)nVal;
    if ((pSpr->cstat & nVal) == 0)
        return 0;
    
    pSpr->cstat &= ~((short)nVal);
    return 1;
}

char sprFlagsAdd(spritetype* pSpr, int nVal)
{
    nVal = (short)nVal;
    if ((pSpr->flags & nVal) != 0)
        return 0;
    
    pSpr->flags |= ((short)nVal);
    return 1;
}

char sprFlagsRem(spritetype* pSpr, int nVal)
{
    nVal = (short)nVal;
    if ((pSpr->flags & nVal) == 0)
        return 0;
    
    pSpr->flags &= ~((short)nVal);
    return 1;
}

char sprSizeSet(spritetype* pSpr, int nVal)
{
    pSpr->xrepeat = pSpr->yrepeat = nVal;
    return 1;
}

char sprSizeXChg(spritetype* pSpr, int nVal)
{
    int v = pSpr->xrepeat;
    if ((nVal < 0 && v <= 1) || (nVal > 0 && v >= 255))
        return 0;
    
    pSpr->xrepeat = ClipRange(v + nVal, 1, 255);
    return 1;
}

char sprSizeYChg(spritetype* pSpr, int nVal)
{
    int v = pSpr->yrepeat;
    if ((nVal < 0 && v <= 1) || (nVal > 0 && v >= 255))
        return 0;
    
    pSpr->yrepeat = ClipRange(v + nVal, 1, 255);
    return 1;
}

char sprOffsXChg(spritetype* pSpr, int nVal)
{
    if ((pSpr->cstat & kSprRelMask) == kSprSloped)
        return 0;
    
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    d = -d;
    
    if (pSpr->cstat & kSprFlipX)
        d = -d;
    
    pSpr->xoffset = changechar(pSpr->xoffset, d, v != 1, 0);
    return 1;
}

char sprOffsYChg(spritetype* pSpr, int nVal)
{
    if ((pSpr->cstat & kSprRelMask) == kSprSloped)
        return 0;
    
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    if ((pSpr->cstat & kSprRelMask) != kSprFace) // fix wrong offset direction when sprite flipped
    {
        if (pSpr->cstat & kSprFlipY)
            d = -d;
    }
    
    pSpr->yoffset = changechar(pSpr->yoffset, d, v != 1, 0);
    return 1;
}

char sprPosXChg(spritetype* pSpr, int nVal)
{
    int v = klabs(nVal);
    int a = (((ED2D) ? 1536 : ang) + kAng90) & kAngMask;
    offsetPos(0, nVal, 0, a, &pSpr->x, &pSpr->y, NULL);
    return 1;
}

char sprPosYChg(spritetype* pSpr, int nVal)
{
    int a = (ED2D) ? 1536 : ang;
    offsetPos(0, nVal, 0, a, &pSpr->x, &pSpr->y, NULL);
    return 1;
}

char spr_Floor_OneSide_HandleFlip(spritetype* pSpr, int nVal)
{
    if (pSpr->cstat & kSprOneSided)
    {
        pSpr->cstat &= ~kSprFlipY;
        if ((pSpr->cstat & kSprRelMask) >= kSprFloor)
        {
            if ((ED2D && gScreen2D.prefs.showMap == 2)
                || (ED3D && nVal > pSpr->z))
                    pSpr->cstat |= kSprFlipY;
        }
        
        return 1;
    }
    
    return 0;
}

char sprSlopeSet(spritetype* pSpr, int nVal)
{
    switch(pSpr->cstat & kSprRelMask)
    {
        case kSprFloor:
        case kSprSloped:
            spriteSetSlope(pSpr->index, nVal);
            return 1;
    }
    
    return 0;
}

char sprSlopeChg(spritetype* pSpr, int nVal)
{
    int nSlope = spriteGetSlope(pSpr->index);
    int v = klabs(nVal);
    
    return sprSlopeSet(pSpr, (nVal < 0) ? IncNext(nSlope, v, 32767) : DecNext(nSlope, v, -32768));
}

char sprSlopeSetFloor(spritetype* pSpr, int)
{
    return sprSlopeSet(pSpr, sector[pSpr->sectnum].floorslope);
}

char sprSlopeSetCeil(spritetype* pSpr, int)
{
    return sprSlopeSet(pSpr, sector[pSpr->sectnum].ceilingslope);
}

char sprDetailSet(spritetype* pSpr, int nVal)
{
    pSpr->detail = nVal;
    return 1;
}

char sprClipdistSet(spritetype* pSpr, int nVal)
{
    pSpr->clipdist = nVal;
    return 1;
}

char sprPicnumSet(spritetype* pSpr, int nVal)
{
    pSpr->picnum = nVal;
    return 1;
}

char sprStateSet(spritetype* pSpr, int nVal)
{
    int n;
    
    if ((n = pSpr->extra) > 0)
    {
        xsprite[n].state = nVal;
        xsprite[n].busy = xsprite[n].state << 16;
        return 1;
    }
    
    return 0;
}

char sprFlipSet(spritetype* pSpr, int nVal)
{
    int zto, zbo, zt, zb;
    
    GetSpriteExtents(pSpr, &zto, &zbo);

    pSpr->cstat &= ~(kSprFlipX|kSprFlipY);
    pSpr->cstat |= nVal;
    
    GetSpriteExtents(pSpr, &zt, &zb);
    pSpr->z += (zto-zt); // compensate Z (useful for wall sprites)
    return 1;
}

char sprFixSector(spritetype* pSpr, int flags)
{
    int x = pSpr->x, y = pSpr->y;
    int cz, fz, i, s, e, z;

    if (inside(x, y, pSpr->sectnum) != 1)
    {
        z = ((tilesizy[pSpr->picnum]*pSpr->yrepeat)<<2);
        for (i = 0; i < numsectors; i++)
        {
            if (inside(x, y, i))
            {
                getzsofslope(i, x, y, &cz, &fz);
                if (pSpr->z >= cz && pSpr->z-z <= fz)
                {
                    ChangeSpriteSect(pSpr->index, i);
                    break;
                }
            }
        }
    }

    if (pSpr->statnum == kStatMarker && pSpr->type != kMarkerWarpDest)
    {
        if (rngok(pSpr->owner, 0, numsectors) && pSpr->sectnum != pSpr->owner)
        {
            if (flags & 0x01)
            {
                ChangeSpriteSect(pSpr->index, pSpr->owner);
                return 1;
            }

            getSectorWalls(pSpr->owner, &s, &e);
            while(s <= e)
            {
                int x1, y1, x2, y2;
                getWallCoords(s, &x1, &y1, &x2, &y2);
                if (pointOnLine(x, y, x1, y1, x2, y2))
                {
                    ChangeSpriteSect(pSpr->index, pSpr->owner);
                    break;
                }

                s++;
            }
        }
    }

    return 1;
}

char sprDelete(spritetype* pSpr, int)
{
    hgltRemove(OBJ_SPRITE, pSpr->index);
    if (pSpr->statnum < kMaxStatus)
    {
        if (pSpr->statnum == kStatPathMarker && pSpr->extra > 0)
        {
            XSPRITE* pXSpr = &xsprite[pSpr->extra];
            XSPRITE* pXNxt = pathMarkerFind(NULL, pXSpr, 1);
            XSPRITE* pXPrv = pathMarkerFind(NULL, pXSpr, 0);

            if (pXNxt || pXPrv)
                pathMarkerChangeData(pXSpr->data1, (pXNxt) ? pXNxt->data1 : pXPrv->data2);
        }
        
        DeleteSprite(pSpr->index);
        return 1;
    }
    
    return 0;
}

char sprPalSet(spritetype* pSpr, int value)
{
    pSpr->pal = (char)ClipRange(value, 0, 255);
    return 1;
}

char sprShadeSet(spritetype* pSpr, int value)
{
    setShadeOf(value, OBJ_SPRITE, pSpr->index);
    return 1;
}

char sprShadeChg(spritetype* pSpr, int step)
{
    iterShadeOf(step, OBJ_SPRITE, pSpr->index);
    return 1;
}

char sprClone(spritetype* pSpr, int)
{
    int i;
    if ((i = InsertSprite(pSpr->sectnum, pSpr->statnum)) >= 0)
    {
        short sect = sprite[i].sectnum;
        memcpy(&sprite[i], pSpr, sizeof(spritetype));
        sprite[i].sectnum = sect;
        sprite[i].index = (short)i;

        ChangeSpriteSect((short)i, sect);

        if (sprInHglt(pSpr->index))
        {
            hgltRemove(OBJ_SPRITE, pSpr->index);
            hgltAdd(OBJ_SPRITE, sprite[i].index);
        }
        
        return 1;
    }
    
    return 0;
}


////////////////////////////////////////////////////
/***********************************************/
/** WALL FUNCTIONS                            **/
/***********************************************/
//////////////////


char wallCstatAdd(int nWall, int nVal)
{
    nVal = (short)nVal;
    if ((wall[nWall].cstat & nVal) != 0)
        return 0;
    
    wall[nWall].cstat |= nVal;
    return 1;
}

char wallCstatRem(int nWall, int nVal)
{
    nVal = (short)nVal;
    if ((wall[nWall].cstat & nVal) == 0)
        return 0;
    
    wall[nWall].cstat &= ~nVal;
    return 1;
}

char wallCstatAdd_OneWay(int nWall, int nVal)
{
    if (wall[nWall].nextwall >= 0)
    {
        nVal = wallCstatAdd(nWall, kWallOneWay);
        if ((nVal & kWallOneWay) && !(nVal & kWallMasked))
            wall[nWall].overpicnum = wall[nWall].picnum;
        
        return 1;
    }
    
    return 0;
}

char wallCstatAdd_Block(int nWall, int nVal)
{
    return (wall[nWall].nextwall >= 0) ? wallCstatAdd(nWall, nVal) : 0;
}

char wallCstatAdd_Mask(int nWall, int)
{
    int n;
    if ((n = wall[nWall].nextwall) >= 0)
    {
        wallCstatAdd(nWall, kWallMasked);
        
        // other side flip-x
        if ((wall[nWall].cstat & kWallFlipX) == 0)
            wallCstatAdd(n, kWallFlipX);
        else if (wall[n].cstat & kWallFlipX) // other side unflip-x
            wallCstatRem(n, kWallFlipX);
        
        wall[nWall].overpicnum = ClipLow(wall[nWall].overpicnum, 0);
        wall[n].overpicnum = wall[nWall].overpicnum;

        // useless to have this together
        wallCstatRem(nWall, kWallOneWay);
        return 1;
    }
    
    return 0;
}

char wallPalSet(int nWall, int nVal)
{
    setPluOf(nVal, OBJ_WALL, nWall);
    return 1;
}

char wallShadeSet(int nWall, int nVal)
{
    setShadeOf(nVal, OBJ_WALL, nWall);
    return 1;
}

char wallPicnumSet(int nWall, int nVal)
{
    wall[nWall].picnum = nVal;
    return 1;
}

char wallPicnumMaskSet(int nWall, int nVal)
{
    wall[nWall].overpicnum = nVal;
    return 1;
}

char wallPanXChg(int nWall, int nVal)
{
    walltype* pWall = &wall[nWall];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    // fix wrong panning direction when wall x-flipped and/or bottom swapped.
    if ((pWall->nextwall >= 0 && (wall[pWall->nextwall].cstat & kWallSwap)
        && (wall[pWall->nextwall].cstat & kWallFlipX)) || (pWall->cstat & kWallFlipX))
            d = -d;
    
    pWall->xpanning = changechar(pWall->xpanning, d, v != 1, 0);
    return 1;
}

char wallPanYChg(int nWall, int nVal)
{
    walltype* pWall = &wall[nWall];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    pWall->ypanning = changechar(pWall->ypanning, -d, v != 1, 0);
    return 1;
}

char wallPanSet(int nWall, int nVal)
{
    wall[nWall].xpanning = wall[nWall].ypanning = nVal;
    return 1;
}

char wallSizeSet(int nWall, int nVal)
{
    wall[nWall].xrepeat = wall[nWall].yrepeat = nVal;
    return 1;
}

char wallSizeXChg(int nWall, int nVal)
{
    int v = wall[nWall].xrepeat;
    if ((nVal < 0 && v <= 0) || (nVal > 0 && v >= 255))
        return 0;
    
    wall[nWall].xrepeat = ClipRange(v + nVal, 0, 255);
    return 1;
}

char wallSizeYChg(int nWall, int nVal)
{
    int v = wall[nWall].yrepeat;
    if ((nVal < 0 && v <= 0) || (nVal > 0 && v >= 255))
        return 0;
    
    wall[nWall].yrepeat = ClipRange(v + nVal, 0, 255);
    return 1;
}

char wallSizeReset(int nWall, int)
{
    wall[nWall].xpanning = wall[nWall].ypanning = 0;
    wall[nWall].xrepeat = wall[nWall].yrepeat = 8;
    fixrepeats(nWall);
    return 1;
}

char wallSizeAuto(int nWall, int)
{
    fixrepeats(nWall);
    return 1;
}

char wallGridSnap(int nWall, int nVal)
{
    doGridCorrection(&wall[nWall].x, &wall[nWall].y, nVal);
    return 1;
}

char wallStateSet(int nWall, int nVal)
{
    int n;
    
    if ((n = sector[nWall].extra) > 0)
    {
        xwall[n].state = nVal;
        xwall[n].busy = xwall[n].state << 16;
        return 1;
    }
    
    return 0;
}

char wallHitagSet(int nWall, int nVal)
{
    wall[nWall].hitag = nVal;
    return 1;
}


////////////////////////////////////////////////////
/***********************************************/
/** FLOOR FUNCTIONS                           **/
/***********************************************/
//////////////////


char floorZSet(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect]; spritetype* pSpr;
    XSECTOR* pXSect = GetXSect(pSect);
    
    char zMotion, zOffset, zIgnore;
    int fs = pSect->floorheinum;
    int cs = pSect->ceilingheinum;
    int i, zt, zb, fz, cz;
    
    // don't allow to go through the ceiling
    if ((fs >= 0 && cs < 0) || (fs < 0 && cs >= 0) || (((fs < 0 && cs < 0) || (fs > 0 && cs > 0)) && fs != cs))
    {
        sectGetEdgeZ(nSect, &fz, &cz); // get most bottom and top Z
        nVal = ClipLow(nVal, cz + (pSect->floorz - fz));
    }
    else
    {
        nVal = ClipLow(nVal, pSect->ceilingz);
    }
    
    if (!IsHoverSprite())
    {
        switch(gMouse.hold & 0x03)
        {
            case 0x02: zOffset = 1; break;
            case 0x03: zIgnore = 1; break;
        }
    }
    
    zMotion = (pXSect && pXSect->onFloorZ != pXSect->offFloorZ);
    
    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        pSpr = &sprite[i];
        GetSpriteExtents(pSpr, &zt, &zb);
        if ((!zIgnore && zb >= getflorzofslope(nSect, pSpr->x, pSpr->y))
            || ((pSpr->cstat & kSprMoveMask) == kSprMoveForward && zMotion) || zOffset)
                pSpr->z += nVal - pSect->floorz;

    }
    
    pSect->floorz = nVal;
    return 1;
}

char floorZChgNext(int nSect, int nVal)
{
    int z = sector[nSect].floorz;
    int v = klabs(nVal);
    
    return floorZSet(nSect, (nVal < 0) ? DecNext(z, v) : IncNext(z, v));
}

char floorZChg(int nSect, int nVal)
{
    return floorZSet(nSect, sector[nSect].floorz + nVal);
}

char floorSlopeSet(int nSect, int nVal)
{
    spritetype* pSpr; sectortype* pSect;
    int i, z, zt, zb; 
    
    pSect = &sector[nSect];
    if (pSect->alignto)
        return 0;
    
    pSect->floorslope = (short)nVal;

    if (pSect->floorslope == 0)
        sectCstatRem(nSect, kSectSloped, OBJ_FLOOR);
    else
        sectCstatAdd(nSect, kSectSloped, OBJ_FLOOR);

    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        pSpr = &sprite[i];
        GetSpriteExtents(pSpr, &zt, &zb);
        z = getflorzofslope(nSect, pSpr->x, pSpr->y);
        if (zb > z)
            pSpr->z += z - zb;
    }
    
    return 1;
}

char floorSlopeChg(int nSect, int nVal)
{
    int nSlope = sector[nSect].floorslope;
    int v = klabs(nVal);
    
    return floorSlopeSet(nSect, (nVal < 0) ? IncNext(nSlope, v, 32767) : DecNext(nSlope, v, -32768));
}

char floorStatAdd(int nSect, int nVal)
{
    nVal = (short)nVal;
    if ((sector[nSect].floorstat & nVal) != 0)
        return 0;

    sector[nSect].floorstat |= nVal;
    return 1;
}

char floorStatRem(int nSect, int nVal)
{
    nVal = (short)nVal;
    if ((sector[nSect].floorstat & nVal) == 0)
        return 0;

    sector[nSect].floorstat &= ~nVal;
    return 1;
}

char floorShadeSet(int nSect, int nVal)
{
    sector[nSect].floorshade = nVal;
    if (isSkySector(nSect, OBJ_FLOOR))
        Sky::SetShade(nSect, OBJ_FLOOR, nVal, 0);

    return 1;
}

char floorPalSet(int nSect, int nVal)
{
    sector[nSect].floorpal = nVal;
    if (isSkySector(nSect, OBJ_FLOOR))
        Sky::SetPal(nSect, OBJ_FLOOR, nVal, 0);
    
    return 1;
}

char floorPicnumSet(int nSect, int nVal)
{
    sector[nSect].floorpicnum = nVal;
    if (isSkySector(nSect, OBJ_FLOOR))
        Sky::SetPic(nSect, OBJ_FLOOR, nVal, !gMisc.diffSky);
    
    return 1;
}

char floorPanSet(int nSect, int nVal)
{
    sector[nSect].floorxpanning = nVal;
    sector[nSect].floorypanning = nVal;
    return 1;
}

char floorPanXChg(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    pSect->floorxpanning = changechar(pSect->floorxpanning, d, v != 1, 0);
    return 1;
}

char floorPanYChg(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    pSect->floorypanning = changechar(pSect->floorypanning, d, v != 1, 0);
    return 1;
}


////////////////////////////////////////////////////
/***********************************************/
/** CEILING FUNCTIONS                         **/
/***********************************************/
//////////////////


char ceilZSet(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect]; spritetype* pSpr;
    XSECTOR* pXSect = GetXSect(pSect);
    
    char zMotion, zOffset, zIgnore;
    int fs = pSect->floorheinum;
    int cs = pSect->ceilingheinum;
    int i, zt, zb, fz, cz;
    

    // don't allow to go through the floor
    if ((fs >= 0 && cs < 0) || (fs < 0 && cs >= 0) || (((fs < 0 && cs < 0) || (fs > 0 && cs > 0)) && fs != cs))
    {
        sectGetEdgeZ(nSect, &fz, &cz); // get most bottom and top Z
        nVal = ClipHigh(nVal, fz + (pSect->ceilingz - cz));
    }
    else
    {
        nVal = ClipHigh(nVal, pSect->floorz);
    }

    zMotion = (pXSect && pXSect->onCeilZ != pXSect->offCeilZ);

    if (!IsHoverSprite())
    {
        switch(gMouse.hold & 0x03)
        {
            case 0x02: zOffset = 1; break;
            case 0x03: zIgnore = 1; break;
        }
    }
    
    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        pSpr = &sprite[i];
        GetSpriteExtents(pSpr, &zt, &zb);
        if ((!zIgnore && zt <= getceilzofslope(nSect, pSpr->x, pSpr->y))
            || ((pSpr->cstat & kSprMoveMask) == kSprMoveReverse && zMotion) || zOffset)
                pSpr->z += nVal - pSect->ceilingz;
    }
    
    pSect->ceilingz = nVal;
    return 1;
}

char ceilZChgNext(int nSect, int nVal)
{
    int z = sector[nSect].ceilingz;
    int v = klabs(nVal);
    
    return ceilZSet(nSect, (nVal < 0) ? DecNext(z, v) : IncNext(z, v));
}

char ceilZChg(int nSect, int nVal)
{
    return ceilZSet(nSect, sector[nSect].ceilingz + nVal);
}

char ceilSlopeSet(int nSect, int nVal)
{
    spritetype* pSpr; sectortype* pSect;
    int i, z, zt, zb;
    
    pSect = &sector[nSect];
    if (pSect->alignto)
        return 0;
    
    pSect->ceilingslope = (short)nVal;

    if (pSect->ceilingslope == 0)
        sectCstatRem(nSect, kSectSloped, OBJ_CEILING);
    else
        sectCstatAdd(nSect, kSectSloped, OBJ_CEILING);

    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        pSpr = &sprite[i];
        GetSpriteExtents(pSpr, &zt, &zb);
        z = getceilzofslope(nSect, pSpr->x, pSpr->y);
        
        if (zt < z)
            pSpr->z += z - zt;
    }
    
    return 1;
}

char ceilSlopeChg(int nSect, int nVal)
{
    int nSlope = sector[nSect].ceilingslope;
    int v = klabs(nVal);
    
    return ceilSlopeSet(nSect, (nVal < 0) ? IncNext(nSlope, v, 32767) : DecNext(nSlope, v, -32768));
}

char ceilStatAdd(int nSect, int nVal)
{
    nVal = (short)nVal;
    if ((sector[nSect].ceilingstat & nVal) != 0)
        return 0;

    sector[nSect].ceilingstat |= nVal;
    return 1;
}

char ceilStatRem(int nSect, int nVal)
{
    nVal = (short)nVal;
    if ((sector[nSect].ceilingstat & nVal) == 0)
        return 0;

    sector[nSect].ceilingstat &= ~nVal;
    return 1;
}

char ceilShadeSet(int nSect, int nVal)
{
    sector[nSect].ceilingshade = nVal;
    if (isSkySector(nSect, OBJ_CEILING))
        Sky::SetShade(nSect, OBJ_CEILING, nVal, 0);
    
    return 1;
}

char ceilPalSet(int nSect, int nVal)
{
    sector[nSect].ceilingpal = nVal;
    if (isSkySector(nSect, OBJ_CEILING))
        Sky::SetPal(nSect, OBJ_CEILING, nVal, 0);
    
    return 1;
}

char ceilPicnumSet(int nSect, int nVal)
{
    sector[nSect].ceilingpicnum = nVal;
    if (isSkySector(nSect, OBJ_CEILING))
        Sky::SetPic(nSect, OBJ_CEILING, nVal, !gMisc.diffSky);
    
    return 1;
}

char ceilPanSet(int nSect, int nVal)
{
    sector[nSect].ceilingxpanning = nVal;
    sector[nSect].ceilingypanning = nVal;
    return 1;
}

char ceilPanXChg(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    pSect->ceilingxpanning = changechar(pSect->ceilingxpanning, d, v != 1, 0);
    return 1;
}

char ceilPanYChg(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect];
    int d = (nVal < 0) ? -1 : 1;
    int v = klabs(nVal);
    
    pSect->ceilingypanning = changechar(pSect->ceilingypanning, d, v != 1, 0);
    return 1;
}

////////////////////////////////////////////////////
/***********************************************/
/** SECTOR FUNCTIONS                          **/
/***********************************************/
//////////////////


char sectVisibilitySet(int nSect, int nVal)
{
    sector[nSect].visibility = nVal;
    return 1;
}

char sectVisibilityChg(int nSect, int nVal)
{
    sector[nSect].visibility += nVal;
    return 1;
}

char sectUnderwaterSet(int nSect, int nVal)
{
    GetXSector(nSect);
    xsector[sector[nSect].extra].underwater = nVal;
    return 1;
}

char sectOffsetZ(int nSect, int nVal)
{
    sectChgZ(nSect, nVal);
    return 1;
}

char sectDelete(int nSect, int)
{
    sectDelete(nSect, 0, 0, 0, 0);
    return 1;
}

char sectFXSetShadeFlags(int nSect, int)
{
    XSECTOR* pXSect = &xsector[GetXSector(nSect)];
    if (!pXSect->shadeFloor && !pXSect->shadeCeiling && !pXSect->shadeWalls)
    {
        pXSect->shadeFloor      = (isSkySector(nSect, OBJ_FLOOR) == 0);
        pXSect->shadeCeiling    = (isSkySector(nSect, OBJ_CEILING) == 0);
        pXSect->shadeWalls      = 1;
        pXSect->shadeAlways     = 1;
    }
    
    return 1;
}

char sectFXFreqChg(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].shadeFreq = ClipRange(xsector[nXSect].shadeFreq + nVal, 0, 255);
    return 1;
}

char sectFXPhaseChg(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].shadePhase = ClipRange(xsector[nXSect].shadePhase + nVal, 0, 255);
    return 1;
}

char sectFXAmplitudeChg(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].amplitude = ClipRange(xsector[nXSect].amplitude + nVal, -128, 127);
    return 1;
}

char sectFXWaveSet(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].shadeWave = nVal;
    return 1;
}

char sectStateSet(int nSect, int nVal)
{
    int n;
    
    if ((n = sector[nSect].extra) > 0)
    {
        xsector[n].state = nVal;
        xsector[n].busy = xsector[n].state << 16;
        return 1;
    }
    
    return 0;
}

char sectShowOrSetupZMotion(int nSect, int nVal)
{
    sectortype* pSect = &sector[nSect]; XSECTOR* pXSect;
    char capture = (nVal & 0x02) != 0;
    
    if (pSect->type == 0 && capture)
        pSect->type = kSectorZMotion; // auto set type?
    
    switch(pSect->type)
    {
        case kSectorZMotionSprite:
            if (!capture)
                break; // can't preview because of sprites...
            // no break
        case kSectorRotate:
        case kSectorRotateMarked:
        case kSectorSlide:
        case kSectorSlideMarked:
        case kSectorZMotion:
            pXSect = &xsector[GetXSector(nSect)];
            
            if (!capture
                && (pXSect->onFloorZ == pXSect->offFloorZ && pXSect->onCeilZ == pXSect->offCeilZ))
                    break; // nothing to show

            if (nVal &  0x01) // on
            {
                if (capture)
                {
                    // set
                    pXSect->onFloorZ = pSect->floorz;
                    pXSect->onCeilZ  = pSect->ceilingz;
                }
                
                // show
                pSect->floorz   = pXSect->onFloorZ;
                pSect->ceilingz = pXSect->onCeilZ;
            }
            else // off
            {
                if (capture)
                {
                    // set
                    pXSect->offFloorZ = pSect->floorz;
                    pXSect->offCeilZ  = pSect->ceilingz;
                }
                
                // show
                pSect->floorz   = pXSect->offFloorZ;
                pSect->ceilingz = pXSect->offCeilZ;
            }
            
            return 1;
    }
    
    return 0;
}

char sectClone(int nSect, int)
{
    int nGrid = (grid <= 0) ? 10 : grid;
    int i, j, s, e;
    
    i = hgltCheck(OBJ_SECTOR, nSect);
    
    SECTORSAVE sect(nSect);
    if ((j = sect.Load()) < 0)
        return 0;
    
    if (i >= 0)
        highlightsector[i] = j;
    
    sectChgXY(j, 2048>>nGrid, 2048>>nGrid, 0x03);
    getSectorWalls(nSect, &s, &e);
    
    j = sector[j].wallptr;
    while(s <= e)
        gNextWall[j] = wall[s].nextwall, s++, j++;
    
    return 1;
}