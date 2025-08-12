/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to work with objects in a highlight of any case (mostly sprites).
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
#include "hglt.h"
#include "xmpmaped.h"
#include "screen.h"
#include "grdshd.h"

BITARRAY16 hgltspri, hgltwall;
int hgltx1 = 0, hgltx2 = 0, hglty1 = 0, hglty2 = 0;
short hgltType = 0;


/** Auto Red Wall stuff **/
static int ARW_helperLoopFindParentSect(POINT2D* pPoint, int c, BITSECTOR visited);
static void ARW_helperMarkVisitedInside(int nStart, BITSECTOR visited);
static void ARW_helperCollectSectors(int nStart, IDLIST* pList);
static int ARW_helperFindNextWall(int nWall);
/**-----------------------------------------**/

void sprFixSector(spritetype* pSpr, int flags)
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
                return;
            }

            getSectorWalls(pSpr->owner, &s, &e);
            while(s <= e)
            {
                int x1, y1, x2, y2;
                getWallCoords(s, &x1, &y1, &x2, &y2);
                if (pointOnLine(x, y, x1, y1, x2, y2))
                {
                    ChangeSpriteSect(pSpr->index, pSpr->owner);
                    return;
                }

                s++;
            }
        }
    }


}

int hgltSprCount() {

    int total = 0;
    for (int i = 0; i < highlightcnt; i++) {
        if ((highlight[i] & 0xC000) != 0 && hgltSprIsFine(highlight[i] & 16383))
            total++;
    }

    return total;
}

int hgltSprCallFunc(HSPRITEFUNC SpriteFunc, int nData) {

    int retn = 0;
    int i, j;
    if (getHighlightedObject() == 200 && !sprInHglt(searchwall)) {
        SpriteFunc(&sprite[searchwall], nData);
        return 0;
    }

    for (i = highlightcnt - 1; i >= 0; i--) {

        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = highlight[i] & 0x3FFF;
        if (sprite[j].statnum < kMaxStatus) {

            SpriteFunc(&sprite[j], nData);
            retn++;

        }
    }

    return retn;
}


int qsSortByIndexASC(short *nID1, short *nID2) { return *nID1 - *nID2; }

int hgltSectDelete()
{
    int i = highlightsectorcnt;
    int r = i;

    if (i > 1) // sort the highlight to make sure it process the right sector
        qsort(highlightsector, highlightsectorcnt,
            sizeof(highlightsector[0]),
                (int(*)(const void*,const void*))qsSortByIndexASC);

    while(--i >= 0) // start from the bottom of array
        deletesector(highlightsector[i]);

    highlightsectorcnt = -1;
    return r;
}

int hgltSectCallFunc(HSECTORFUNC2 SectorFunc, int arg1, int arg2, int arg3, int arg4)
{
    int i = highlightsectorcnt;
    int r = 0;

    while(--i >= 0)
    {
        SectorFunc(highlightsector[i], arg1, arg2, arg3, arg4);
        r++;
    }

    return r;
}


short hgltCheck(int type, int idx) {

    int i;
    switch (type) {
        case OBJ_WALL:
        case OBJ_MASKED:
            for (i = 0; i < highlightcnt; i++) {
                if ((highlight[i] & 0xC000) != 0) continue;
                else if (highlight[i] == idx) return (short)i;
            }
            break;
        case OBJ_SPRITE:
            for (i = 0; i < highlightcnt; i++) {
                if ((highlight[i] & 0xC000) != 0x4000) continue;
                else if ((highlight[i] & 0x3FFF) == idx) return (short)i;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            for (i = 0; i < highlightsectorcnt; i++) {
                if (highlightsector[i] == idx) return (short)i;
            }
            break;
    }

    return -1;

}

BOOL hgltRemove(int type, int idx) {

    int i = -1;
    switch (type) {
        case OBJ_WALL:
        case OBJ_MASKED:
        case OBJ_SPRITE:
            if (highlightcnt > 0) {

                if ((i = hgltCheck(type, idx)) < 0) return FALSE;
                else if (type != OBJ_SPRITE) ClearBitString(hgltwall, idx);
                else ClearBitString(hgltspri, idx);
                while(i < highlightcnt - 1) {
                    highlight[i] = highlight[i + 1];
                    i++;
                }
                if (--highlightcnt <= 0) highlightcnt = -1;
                return TRUE;

            }
            break;
        case OBJ_SECTOR:
        case OBJ_FLOOR:
        case OBJ_CEILING:
            if (highlightsectorcnt > 0) {

                if ((i = hgltCheck(type, idx)) < 0) return FALSE;
                else sectAttach(idx);
                while (i < highlightsectorcnt - 1) {
                    highlightsector[i] = highlightsector[i + 1];
                    i++;
                }
                if (--highlightsectorcnt <= 0) highlightsectorcnt = -1;
                return TRUE;

            }
            break;
    }

    return FALSE;

}

int hgltAdd(int type, int idx) {

    int retn = -1;
    switch (type) {
        case OBJ_SPRITE:
        case OBJ_WALL:
        case OBJ_MASKED:
            retn = highlightcnt = (short)ClipLow(highlightcnt, 0);
            switch (type) {
                case OBJ_SPRITE:
                    highlight[highlightcnt++] = (short)(idx + 16384);
                    SetBitString(hgltspri, idx);
                    break;
                default:
                    highlight[highlightcnt++] = (short)idx;
                    SetBitString(hgltwall, idx);
                    break;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
        {
            retn = highlightsectorcnt = (short)ClipLow(highlightsectorcnt, 0);
            highlightsector[highlightsectorcnt++] = (short)idx;

            int s, e;
            getSectorWalls(idx, &s, &e);
            while(s <= e)
            {
                if (wall[s].nextwall >= 0)
                    gNextWall[wall[s].nextwall] = wall[wall[s].nextwall].nextwall;

                gNextWall[s] = wall[s].nextwall;
                s++;
            }

            break;
        }

    }

    return retn;

}

short hglt2dAddInXYRange(int hgltType, int x1, int y1, int x2, int y2) {

    int i, j; int swal, ewal, cnt = 0;
    for (i = 0; i < numsectors; i++)
    {
        if (hgltType & kHgltPoint)
        {
            getSectorWalls(i, &swal, &ewal);
            for (j = swal; j <= ewal; j++)
            {
                if (wall[j].x < x1 || wall[j].x > x2) continue;
                else if (wall[j].y < y1 || wall[j].y > y2) continue;
                else hgltAdd(OBJ_WALL, j);
                cnt++;
            }

            for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
            {
                if (sprite[j].statnum >= kMaxStatus) continue;
                else if (sprite[j].x < x1 || sprite[j].x > x2) continue;
                else if (sprite[j].y < y1 || sprite[j].y > y2) continue;
                else hgltAdd(OBJ_SPRITE, j);
                cnt++;
            }
        }

        if (hgltType & kHgltSector)
        {
            getSectorWalls(i, &swal, &ewal);
            for (j = swal; j <= ewal; j++)
            {
                if (wall[j].x < x1 || wall[j].x > x2) break;
                else if (wall[j].y < y1 || wall[j].y > y2) break;
            }

            if (j >= ewal + 1)
            {
                hgltAdd(OBJ_SECTOR, i);
                cnt++;
            }
        }
    }

    return cnt;

}


short hglt2dAdd(int type, int idx) {

    short cnt = 0;
    int i = 0; int x, y, sect;
    switch (type) {
        case OBJ_SPRITE:
        case OBJ_WALL:
        case OBJ_MASKED:
            highlightcnt = (short)ClipLow(highlightcnt, 0);
            switch (type) {
                case OBJ_SPRITE:
                    x = sprite[idx].x; y = sprite[idx].y, sect = sprite[idx].sectnum;
                    for (i = headspritesect[sect]; i >= 0; i = nextspritesect[i])
                    {
                        if (sprite[i].statnum >= kStatFree) continue;
                        else if (sprite[i].x == x && sprite[i].y == y && hgltCheck(type, i) < 0)
                            hgltAdd(type, i), cnt++;
                    }
                    break;
                default:
                    x = wall[idx].x; y = wall[idx].y;
                    for (i = 0; i < numwalls; i++) {
                        if (wall[i].x == x && wall[i].y == y && hgltCheck(type, i) < 0)
                            hgltAdd(type, i), cnt++;
                    }
                    break;
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            if (hgltCheck(type, idx) >= 0) break;
            hgltAdd(type, idx), cnt++;
            break;
    }

    return cnt;

}

short hglt2dRemove(int type, int idx) {

    short cnt = 0;
    int i = 0; int x, y;
    switch (type)
    {
        case OBJ_SPRITE:
            x = sprite[idx].x; y = sprite[idx].y; // exclude all sprites with same xy
            for (i = highlightcnt - 1; i >= 0; i--)
            {
                if ((highlight[i] & 0xC000) == 0x4000)
                {
                    idx = highlight[i] & 0x3FFF;
                    if (sprite[idx].x == x && sprite[idx].y == y)
                        hgltRemove(type, idx), cnt++;
                }
            }
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            x = wall[idx].x; y = wall[idx].y; // exclude all wall points with same xy
            for (i = highlightcnt - 1; i >= 0; i--)
            {
                if ((highlight[i] & 0xC000) == 0)
                {
                    idx = highlight[i];
                    if (wall[idx].x == x && wall[idx].y == y)
                        hgltRemove(type, idx), cnt++;
                }
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
        case OBJ_SECTOR:
            if (hgltRemove(type, idx)) cnt++;
            break;
    }

    return cnt;
}

void hgltReset(int which) {

    int i;
    if ((which & kHgltPoint) && highlightcnt > 0) {

        for (i = highlightcnt - 1; i >= 0; i--) {
            if ((highlight[i] & 0xC000) == 0) hgltRemove(OBJ_WALL, highlight[i]);
            else hgltRemove(OBJ_SPRITE, highlight[i] & 0x3FFF);

        }

        highlightcnt = -1;

    }

    if ((which & kHgltSector) && highlightsectorcnt > 0) {
        for (i = highlightsectorcnt - 1; i >= 0; i--)
            hgltRemove(OBJ_FLOOR, highlightsector[i]);

        highlightsectorcnt = -1;
    }

    if (which & kHgltGradient)
        gListGrd.Clear(); // clear out 3d mode highlights
}

void hgltSprGetEdgeSpr(short* ls, short* rs, short* ts, short* bs, short* zts, short* zbs) {

    short j = -1; int i;
    int zTop = 0, zBot = 0, zb = 0, zt = 0;

    for (i = 0; i < highlightcnt; i++) {

        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = (short) (highlight[i] & 16383);
        if (!hgltSprIsFine(j))
            continue;

        zt  = zb  = sprite[j].z;
        *ls = *rs = *ts = *bs = *zts = *zbs = j;
        j = (short)i;
        break;

    }

    dassert(*ls != -1);

    for (i = j; i < highlightcnt; i++) {
        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = (short) (highlight[i] & 16383);
        if (!hgltSprIsFine(j))
            continue;

        if (sprite[j].x < sprite[*ls].x) *ls = j;
        if (sprite[j].x > sprite[*rs].x) *rs = j;
        if (sprite[j].y < sprite[*bs].y) *bs = j;
        if (sprite[j].y > sprite[*ts].y) *ts = j;

        GetSpriteExtents(&sprite[j], &zTop, &zBot);

        if (zTop <= zt)
        {
            *zts = j;
             zt = zTop;
        }

        if (zBot >= zb)
        {
            *zbs = j;
             zb = zBot;
        }
    }

}

void hgltSprGetZEdgeSpr(short* lowest, short* highest) {

    int zTop = 0, zBot = 0;
    short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
    hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);

    dassert(zts != -1 && zbs != -1);

    if (lowest) *lowest = zbs;
    if (highest) *highest = zts;

}


void hgltSprAvePoint(int* dax, int* day, int* daz) {

    short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
    hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);


    *dax = sprite[ls].x + ((sprite[rs].x - sprite[ls].x) >> 1);
    *day = sprite[ts].y + ((sprite[bs].y - sprite[ts].y) >> 1);
    if (daz != NULL)
        *daz = sprite[zts].z + ((sprite[zbs].z - sprite[zts].z) >> 1);

}

void hgltSprGetEdges(int* left, int* right, int* top, int* bot, int* ztop, int* zbot, int* ztofs, int* zbofs) {

    int zTop = 0, zBot = 0;
    short ls = -1, rs = -1, bs = -1, ts = -1, zbs = -1, zts = -1;
    hgltSprGetEdgeSpr(&ls, &rs, &ts, &bs, &zts, &zbs);

    dassert(ls != -1);

    *left = sprite[ls].x;   *right = sprite[rs].x;
    *top  = sprite[ts].y;   *bot   = sprite[bs].y;
    *ztop = sprite[zts].z;  *zbot  = sprite[zbs].z;

    if (ztofs) {

        *ztofs = 0;
        GetSpriteExtents(&sprite[zts], &zTop, &zBot);
        if (zTop <= sector[sprite[zts].sectnum].ceilingz)
            *ztofs = sector[sprite[zts].sectnum].ceilingz - zTop;

    }

    if (zbofs) {

        *zbofs = 0;
        GetSpriteExtents(&sprite[zbs], &zTop, &zBot);
        if (zBot >= sector[sprite[zbs].sectnum].floorz)
            *zbofs = zBot - sector[sprite[zbs].sectnum].floorz;

    }


}

void sprGetZOffsets(short idx, int* ztofs, int* zbofs) {

    int left = 0, right = 0, top = 0, bottom = 0, ztop = 0, zbottom = 0, ztofs2 = 0, zbofs2 = 0;
    if (!sprInHglt(idx)) {

        int zTop, zBot;
        spritetype* pSprite = &sprite[idx];

        GetSpriteExtents(pSprite, &zTop, &zBot);
        if (zTop <= sector[pSprite->sectnum].ceilingz)
            ztofs2 = sector[pSprite->sectnum].ceilingz - zTop;

        if (zBot >= sector[pSprite->sectnum].floorz)
            zbofs2 = zBot - sector[pSprite->sectnum].floorz;


    } else {

        hgltSprGetEdges(&left, &right, &top, &bottom, &ztop, &zbottom, &ztofs2, &zbofs2);

    }

    *ztofs = ztofs2;
    *zbofs = zbofs2;
    return;

}

// don't allow to go through floors or ceiling keeping the shape
void hgltSprClamp(int ofsAboveCeil, int ofsBelowFloor, int which) {


    short hg = -1, lw = -1;
    int zTop, zBot, value = 0, nz = 0, oz = 0;
    hgltSprGetZEdgeSpr(&lw, &hg);

    if (which & 0x0001) {

        GetSpriteExtents(&sprite[hg], &zTop, &zBot);
        if (zTop <= getceilzofslope(sprite[hg].sectnum, sprite[hg].x, sprite[hg].y))
            hgltSprPutOnCeiling(ofsAboveCeil);

    }


    if (which & 0x0002) {

        GetSpriteExtents(&sprite[lw], &zTop, &zBot);
        if (zBot >= getflorzofslope(sprite[lw].sectnum, sprite[lw].x, sprite[lw].y))
            hgltSprPutOnFloor(ofsBelowFloor);

    }

}

void hglt2List(OBJECT_LIST* pList, char asX, char which)
{
    int i, s, e, t;

    if (which & kHgltSector)
    {
        for (i = 0; i < highlightsectorcnt; i++)
        {
            t = highlightsector[i];
            if (asX)
            {
                if (GetXSect(&sector[t]))
                    pList->Add(OBJ_SECTOR, sector[t].extra);
            }
            else
            {
                pList->Add(OBJ_SECTOR, t);
            }

            getSectorWalls(t, &s, &e);
            while(s <= e)
            {
                if (asX)
                {
                    if (GetXWall(&wall[s]))
                        pList->Add(OBJ_WALL, wall[s].extra);
                }
                else
                {
                    pList->Add(OBJ_WALL, s);
                }

                s++;
            }

            for (s = headspritesect[t]; s >= 0; s = nextspritesect[s])
            {
                if (asX)
                {
                    if (GetXSpr(&sprite[s]))
                        pList->Add(OBJ_SPRITE, sprite[s].extra);
                }
                else
                {
                    pList->Add(OBJ_SPRITE, s);
                }
            }
        }
    }

    if (which & kHgltPoint)
    {
        for (i = 0; i < highlightcnt; i++)
        {
            if ((highlight[i] & 0xC000) == 0x4000)
            {
                s = highlight[i] & 0x3FFF;
                if (asX)
                {
                    if (GetXSpr(&sprite[s]))
                        pList->Add(OBJ_SPRITE, sprite[s].extra);
                }
                else
                {
                    pList->Add(OBJ_SPRITE, s);
                }
            }
            else
            {
                s = highlight[i];
                if (asX)
                {
                    if (GetXWall(&wall[s]))
                        pList->Add(OBJ_WALL, wall[s].extra);
                }
                else
                {
                    pList->Add(OBJ_WALL, s);
                }
            }
        }

        // should we add sectors if all walls of sector in this highlight...?
    }
}

char hgltRelaceChannel(OBJECT_LIST* pList, int nOld, int nNew, char which)
{
    OBJECT* pObj = pList->Ptr();
    int nID, nPrev, i, t, tx[4];
    char rpc = 0;


    while(pObj->type != OBJ_NONE)
    {
        nID = pObj->index;
        switch(pObj->type)
        {
            case OBJ_SECTOR:
                if((which & 0x2) && xsector[nID].txID == nOld) xsector[nID].txID = nNew, rpc = 1;
                if((which & 0x1) && xsector[nID].rxID == nOld) xsector[nID].rxID = nNew, rpc = 1;
                break;
            case OBJ_SPRITE:
                if (which & 0x02)
                {
                    if (xsprite[nID].txID == nOld)
                        xsprite[nID].txID = nNew, rpc = 1;

                    if (isMultiTx(xsprite[nID].reference))
                    {
                        if (multiTxGetRange(xsprite[nID].reference, tx))
                        {
                            for (i = tx[0]; i <= tx[1]; i++)
                            {
                                if (i == nOld)
                                {
                                    // must try to set new range here
                                    if ((nPrev = findUnusedChannel()) >= 0)
                                    {
                                        t = tx[1]-tx[0];
                                        for (i = 0; i < t && !collectObjectsByChannel(++nPrev, 1, NULL, 0x0); i++);
                                        if (i >= t)
                                        {
                                            setDataOf(0, nPrev - t, OBJ_SPRITE, xsprite[nID].reference);    // data1
                                            setDataOf(3, nPrev,     OBJ_SPRITE, xsprite[nID].reference);    // data4
                                            rpc = 1;
                                        }

                                        // !!! assign objects to a new range somehow
                                    }

                                    break;
                                }
                            }
                        }
                        else
                        {
                            for (i = 0; i < 4; i++)
                            {
                                if (tx[i] == nOld)
                                    setDataOf(i, nNew, OBJ_SPRITE, xsprite[nID].reference), rpc = 1;
                            }
                        }
                    }
                }
                if((which & 0x1) && xsprite[nID].rxID == nOld) xsprite[nID].rxID = nNew, rpc = 1;
                break;
            case OBJ_WALL:
                if((which & 0x2) && xwall[nID].txID == nOld) xwall[nID].txID = nNew, rpc = 1;
                if((which & 0x1) && xwall[nID].rxID == nOld) xwall[nID].rxID = nNew, rpc = 1;
                break;
        }

        pObj++;
    }

    return rpc;
}

void hgltIsolateChannels(char which)
{
    unsigned char used[1024]; const int l = LENGTH(used);
    OBJECT_LIST hglt; OBJECT* pObj;
    int i, j, k = 0;

    hglt2List(&hglt, 1, which);
    collectUsedChannels(used);

    for (i = 100; i < l; i++)
    {
        if (k >= LENGTH(used)-100)
        {
            ThrowError("Out of free TX/RX channels!");
            break;
        }


        if (used[i] == 1) // must be originally busy channel
        {
            k++;
            for (j = 100; j < l ; j++)
            {
                if (used[j] == 0 && hgltRelaceChannel(&hglt, i, j, 0x03)) // replace TX and RX of the matching objects
                {
                    if (!collectObjectsByChannel(i, 0, NULL, 0x0))
                        used[i] = 0, k--; // this channel freed

                    used[j] = 2, k++; // new used channel
                    break;
                }
            }
        }
    }
}

void hgltIsolatePathMarkers(int which)
{
    spritetype *pMarkA, *pMarkB; int nOld, nA, nB;
    OBJECT_LIST hglt; OBJECT *a, *b;
    hglt2List(&hglt, 1, which);


    a = hglt.Ptr();
    while(a->type != OBJ_NONE)
    {
        nA = a->index;
        if (a->type == OBJ_SPRITE)
        {
            pMarkA = &sprite[xsprite[nA].reference];
            if (pMarkA->type == kMarkerPath)
            {
                nOld = xsprite[nA].data1;
                if ((xsprite[nA].data1 = findUnusedPath()) < 0)
                    ThrowError("Out of free path ID!");

                // search all markers that have data1 == nOld or data2 == nOld and replace it

                b = hglt.Ptr();
                while(b->type != OBJ_NONE)
                {
                    nB = b->index;
                    if (b->type == OBJ_SPRITE)
                    {
                        pMarkB = &sprite[xsprite[nB].reference];
                        if (pMarkB->type == kMarkerPath)
                        {
                            if (xsprite[nB].data1 == nOld)
                                xsprite[nB].data1 = xsprite[nA].data1;

                            if (xsprite[nB].data2 == nOld)
                                xsprite[nB].data2 = xsprite[nA].data1;
                        }
                    }
                    else if (b->type == OBJ_SECTOR)
                    {
                        // also replace data for path sectors if any
                        sectortype* pSect = &sector[xsector[nB].reference];
                        if (pSect->type == kSectorPath && xsector[nB].data == nOld)
                            xsector[nB].data = xsprite[nA].data1;
                    }

                    b++;
                }
            }
        }

        a++;
    }
}

void hgltIsolateRorMarkers(int which)
{
    spritetype *pMarkA, *pMarkB; int nOld, nA, nB;
    OBJECT_LIST hglt; OBJECT *a, *b;
    hglt2List(&hglt, 1, which);

    a = hglt.Ptr();
    while(a->type != OBJ_NONE)
    {
        nA = a->index;
        if (a->type == OBJ_SPRITE)
        {
            pMarkA = &sprite[xsprite[nA].reference];
            if (irngok(pMarkA->type, kMarkerLowLink, kMarkerLowGoo) && pMarkA->type != kMarkerWarpDest)
            {
                nOld = xsprite[nA].data1;
                if ((xsprite[nA].data1 = findUnusedStack()) < 0)
                    ThrowError("Out of free ROR ID!");

                // search all markers that have data1 == nOld and replace it

                b = hglt.Ptr();
                while(b->type != OBJ_NONE)
                {
                    nB = b->index;
                    if (b->type == OBJ_SPRITE)
                    {
                        pMarkB = &sprite[xsprite[nB].reference];
                        if (irngok(pMarkB->type, kMarkerLowLink, kMarkerLowGoo) && pMarkB->type != kMarkerWarpDest)
                        {
                            if (xsprite[nB].data1 == nOld)
                                xsprite[nB].data1 = xsprite[nA].data1;
                        }
                    }

                    b++;
                }
            }
        }

        a++;
    }
}

void hgltSprSetXYZ(int x, int y, int z) {

    int dax, day, daz;
    hgltSprAvePoint(&dax, &day, &daz);
    hgltSprChgXYZ(x - dax, y - day, z - daz);

}

void hgltSprChgXYZ(int xstep, int ystep, int zstep) {

    int i, j;
    for (i = 0; i < highlightcnt; i++) {
        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = highlight[i] & 16383;
        if (sprite[j].statnum >= kStatFree)
            continue;

        if (xstep) sprite[j].x += xstep;
        if (ystep) sprite[j].y += ystep;
        if (zstep) sprite[j].z += zstep;
    }

    hgltSprCallFunc(sprFixSector);

}

// put sprites on floor keeping the shape
void hgltSprPutOnFloor(int ofs) {

    short lowest = -1;
    hgltSprGetZEdgeSpr(&lowest, NULL);
    if (lowest < 0)
        return;

    int oz = sprite[lowest].z;
    PutSpriteOnFloor(&sprite[lowest], 0);
    int nz = sprite[lowest].z;
    sprite[lowest].z = oz;

    hgltSprChgXYZ(0, 0,(nz - oz) + ofs);

}


// put sprites on ceiling keeping the shape
void hgltSprPutOnCeiling(int ofs) {

    short highest = -1;
    hgltSprGetZEdgeSpr(NULL, &highest);
    if (highest < 0)
        return;

    int oz = sprite[highest].z;
    PutSpriteOnCeiling(&sprite[highest], 0);
    int nz = sprite[highest].z;
    sprite[highest].z = oz;

    hgltSprChgXYZ(0, 0, (nz - oz) - ofs);

}

void hgltSprPutOnZ(int z, int which, int tofs, int bofs) {

    short hg = -1, lw = -1;
    int zTop, zBot, value = 0, nz = 0, oz = 0;
    hgltSprGetZEdgeSpr(&lw, &hg);

    if (hg >= 0 && (which & 0x0001)) {

        oz = sprite[hg].z;
        GetSpriteExtents(&sprite[hg], &zTop, &zBot);

        sprite[hg].z += z - zTop; nz = sprite[hg].z;
        sprite[hg].z = oz;

        hgltSprChgXYZ(0, 0, (nz - oz) - tofs);
    }

    if (lw >= 0 && (which & 0x0002)) {

        oz = sprite[lw].z;
        GetSpriteExtents(&sprite[lw], &zTop, &zBot);

        sprite[lw].z += z - zBot; nz = sprite[lw].z;
        sprite[lw].z = oz;

        hgltSprChgXYZ(0, 0, (nz - oz) + bofs);
    }

}

void hgltSprPutOnWall(int nwall, int x, int y)
{
    spritetype* pSpr;
    int nAng, ex[4], ey[4];
    int x1, y1, x2, y2;
    int i, j, k, e;

    getWallCoords(nwall, &x1, &y1, &x2, &y2);
    nAng = getangle(x2-x1, y2-y1);

    for (i = 0; i < highlightcnt; i++)
    {
        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = highlight[i] & 16383;
        pSpr = &sprite[j];
        e = 0;

        while(e >= 0)
        {
            switch(pSpr->cstat & kSprRelMask)
            {
                case kSprSloped:
                case kSprFloor:
                    GetSpriteExtents(pSpr, &ex[0], &ey[0], &ex[1], &ey[1], &ex[2], &ey[2], &ex[3], &ey[3]);
                    e = 4;
                    break;
                case kSprWall:
                    GetSpriteExtents(pSpr, &ex[0], &ey[0], &ex[1], &ey[1]);
                    e = 2;
                    break;
                default:
                    ex[0] = pSpr->x;
                    ey[0] = pSpr->y;
                    e = 1;
                    break;
            }

            while(--e >= 0)
            {
                // don't know how to make it better
                if (pointBehindLine(ex[e], ey[e], x1, y1, x2, y2))
                {
                    for (k = 0; k < highlightcnt; k++)
                    {
                        if ((highlight[k] & 0xC000) == 0)
                            continue;

                        // j is safe for use
                        j = highlight[k] & 16383;
                        offsetPos(0, 12, 0, nAng + kAng90,
                                    &sprite[j].x, &sprite[j].y, NULL);
                    }

                    break;
                }
            }
        }
    }

    hgltSprCallFunc(sprFixSector);

}

void hgltSprRotate(int step) {

    int i, j;
    int dax = 0, day = 0;


    hgltSprAvePoint(&dax, &day);


    for (i = 0; i < highlightcnt; i++) {

        if ((highlight[i] & 0xC000) == 0)
            continue;

        j = highlight[i] & 16383;
        if (sprite[j].statnum < kStatFree) {

            RotatePoint(&sprite[j].x, &sprite[j].y, step, dax, day);

            int nType = sprite[j].type; // check if allowed to change markers angle...
            if ((nType >= 3 && nType <= 5) && !(gRotateOpts.chgMarkerAng & 0x0002)) continue;
            else if ((nType == 15) && !(gRotateOpts.chgMarkerAng & 0x0001)) continue;

            sprite[j].ang = (short)(sprite[j].ang + step);
            if (!isMarkerSprite(j))
                sprite[j].ang = sprite[j].ang & kAngMask;

            sprFixSector(&sprite[j], 0);

        }

    }

}

int hgltWallCount(int* whicnt, int* redcnt)
{
    int i, j, total = 0;
    if (whicnt) *whicnt = 0;
    if (redcnt) *redcnt = 0;
    for (i = 0; i < highlightcnt; i++)
    {
        if ((highlight[i] & 0xC000) != 0)
            continue;

        total++;
        j = highlight[i];
        if (wall[j].nextwall >= 0 && redcnt != NULL) *redcnt = *redcnt + 1;
        else if (whicnt != NULL)
            *whicnt = *whicnt + 1;
    }

    return total;
}

int hgltWallsCheckStat(int nStat, int which, int nMask)
{
    int i, nwall, j = 0, red, redcnt = 0, whicnt = 0, cstat;
    if (!hgltWallCount(&whicnt, &redcnt))
        return 0;

    for (i = 0; i < highlightcnt; i++)
    {
        if ((highlight[i] & 0xC000) != 0)
            continue;

        nwall = highlight[i];
        red = (wall[nwall].nextwall >= 0);
        cstat = wall[nwall].cstat;

        if (((which & 0x01) && !red) || ((which & 0x02) && red))
        {
            if (which & 0x04)
            {
                if ((nMask && (cstat & nMask) != nStat) || (!nMask && !(cstat & nStat)))
                    j++;
            }
            else if ((nMask && (cstat & nMask) == nStat) || (!nMask && (cstat & nStat)))
                j++;
        }
    }

    if ((which & 0x03) == 0x03) return (j + redcnt + whicnt) - (redcnt + whicnt);
    else if (which & 0x01) return (j + whicnt) - whicnt;
    else if (which & 0x02) return (j + redcnt) - redcnt;
    return 0;
}

void hgltSectGetBox(int* x1, int* y1, int* x2, int *y2)
{
    int nFirst = sector[highlightsector[0]].wallptr;
    int i = highlightsectorcnt;
    int tx1, ty1, tx2, ty2;
    int s, e;

    *x1 = *x2 = wall[nFirst].x;
    *y1 = *y2 = wall[nFirst].y;

    while(--i >= 0)
    {
        getSectorWalls(highlightsector[i], &s, &e);
        loopGetBox(s, e, &tx1, &ty1, &tx2, &ty2);
        if (tx1 < *x1) *x1 = tx1;
        if (tx2 > *x2) *x2 = tx2;

        if (ty1 < *y1) *y1 = ty1;
        if (ty2 > *y2) *y2 = ty2;
    }
}

void hgltSectMidPoint(int* ax, int* ay)
{
    int x1, y1, x2, y2;
    hgltSectGetBox(&x1, &y1, &x2, &y2);

    *ax = x1 + ((x2 - x1) >> 1);
    *ay = y1 + ((y2 - y1) >> 1);
}

char hgltSectInsideBox(int x, int y)
{
    if (highlightsectorcnt < 0)
        return 0;

    int x1, y1, x2, y2;
    hgltSectGetBox(&x1, &y1, &x2, &y2);
    return (irngok(x, x1, x2) && irngok(y, y1, y2));
}

void hgltSectDetach()
{
    int i = highlightsectorcnt;
    int n, s, e;

    BITARRAY16 done;
    memset(done, 0, sizeof(done));

    while(--i >= 0)
    {
        getSectorWalls(highlightsector[i], &s, &e);
        while(s <= e)
        {
            n = wall[s].nextsector;
            if (n >= 0 && (TestBitString(done, n) || hgltCheck(OBJ_SECTOR, n) < 0))
            {
                SetBitString(done, n);
                wallDetach(wall[s].nextwall);
                wallDetach(s);
            }

            s++;
        }
    }
}

void hgltSectAttach()
{
    int i = highlightsectorcnt;
    int n, t, s, e;

    while(--i >= 0)
    {
        t = highlightsector[i];
        getSectorWalls(t, &s, &e);
        while(s <= e)
        {
            if (wall[s].nextwall < 0 && (n = findNextWall(s)) >= 0)
            {
                if (wall[n].nextwall < 0)
                {
                    wallAttach(s, sectorofwall(n), n);
                    wallAttach(n, t, s);
                }
            }

            s++;
        }
    }
}

int hgltSectFlip(int flags)
{
    int cx, cy;
    hgltSectMidPoint(&cx, &cy);
    hgltSectDetach();

    if (flags & 0x04)
    {
        IDLIST done(true);
        int nSect = 0;
        int s, e;

        // first flip outer loops of whole highlight
        while(hgltListOuterLoops(&nSect, &s, &e, &done, kHgltSector))
            loopFlip(s, e, cx, cy, flags);
    }

    // flip sectors inside highlight
    int r = hgltSectCallFunc(sectFlip, cx, cy, flags);
    hgltSectAttach();
    return r;
}

int hgltSectRotate(int flags, int nAng)
{
    int cx, cy;
    hgltSectMidPoint(&cx, &cy);
    hgltSectDetach();

    if (flags & 0x04)
    {
        IDLIST done(true);
        int nSect = 0;
        int s, e;

        // first rotate outer loops of whole highlight
        while(hgltListOuterLoops(&nSect, &s, &e, &done, kHgltSector))
            loopRotate(s, e, cx, cy, nAng, flags);
    }

    // rotate sectors inside highlight
    int r = hgltSectCallFunc(sectRotate, cx, cy, nAng, flags);
    hgltSectAttach();
    return r;
}

int hgltListInnerLoops(IDLIST* pLoop, BITSECTOR visited, char which)
{
    int i = highlightsectorcnt, c = 0;
    int nSect, nWall = -1, s, e, t;
    IDLIST* pNode; int32_t* p;

    while(--i >= 0)
    {
        t = highlightsector[i];
        if (TestBitString(visited, t))
            continue;

        getSectorWalls(t, &s, &e);
        while(s <= e)
        {
            // keep searching the most left one-sided wall
            if (wall[s].nextwall < 0 && (nWall < 0 || wall[s].x < wall[nWall].x))
                nWall = s, nSect = t;

            s++;
        }
    }

    if (nWall < 0)
        return -1;

    //Alert("CUR SECT: %d", nSect);
    SetBitString(visited, nSect);
    i = nWall;

    do
    {
        i = wall[i].point2;
        if (wall[i].nextwall >= 0)
        {
            pNode = new IDLIST(true);
            if (collectWallsOfNode(pNode, i) > 2)
            {
                for (p = pNode->GetPtr(); *p >= 0; p++)
                {
                    if (wall[*p].nextwall < 0)
                    {
                        i = *p;
                        break;
                    }
                }

                if ((t = sectorofwall(i)) >= 0)
                    SetBitString(visited, t);
            }

            delete(pNode);
        }

        if (!pLoop->Exists(i))
            pLoop->Add(i);

        c++;
    }
    while(i != nWall && c < numwalls);
    return (c < numwalls) ? nSect : -2;
}

char hgltListOuterLoops(int* nStart, int* s, int* e, IDLIST* pDone, char which)
{
    IDLIST loop(true); BITSECTOR ignore;
    int i, nSect, nParent = -1, nNext;
    int32_t* p;

    memset(ignore, 0, sizeof(ignore));
    *nStart = 0; // this is dummy

    if (pDone)
    {
        p = pDone->GetPtr();
        while(*p >= 0)
            SetBitString(ignore, *p), p++;
    }

    if ((nSect = hgltListInnerLoops(&loop, ignore, which)) >= 0)
    {
        ARW_helperCollectSectors(nSect, pDone);

        p = loop.GetPtr();
        while(*p >= 0 && (nNext = findNextWall(*p)) >= 0) // make sure it's an island loop
        {
            if ((nSect = sectorofwall(nNext)) < 0)
                break;

            if (nParent < 0) nParent = nSect;
            else if (nSect != nParent)
                break;

            p++;
        }

        if (*p < 0)
        {
            loopGetWalls(nNext, s, e); // got some outer loop
            return 1;
        }
    }

    return 0;
}

int hgltSectAutoRedWall(int flags)
{
    IDLIST loop(true); BITSECTOR ignore;
    int nLoop, nLoopsDone = 0, numloops = 0;
    int nSrc, nNext, nDst, nWall = 0;
    int nSect, nParent, i, t, c = 0;
    int32_t *p;

    char zOffsetsDone = 0, parentFound = 0;
    walltype *pWalls, *pSrc, *pNext, *pDst;
    sectortype* pParent; POINT2D* pPoint;
    memset(ignore, 0, sizeof(ignore));

    // List all the loops in a highlight
    while((nSect = hgltListInnerLoops(&loop, ignore, kHgltSector)) >= 0)
    {
        loop.Add(kMaxWalls); // add loop separator
        ARW_helperMarkVisitedInside(nSect, ignore);
        numloops++;
    }


    if (numloops <= 0)
        return 0;

    // Allocate storage for info
    t = loop.GetLength() - numloops;
    pParent = (sectortype*)malloc(sizeof(sectortype) * numloops);   dassert(pParent != NULL);
    pWalls = (walltype*)malloc(sizeof(walltype) * t);               dassert(pWalls != NULL);
    pPoint = (POINT2D*)malloc(sizeof(POINT2D) * t);                 dassert(pPoint != NULL);

    // Grab all the required info such as appearance,
    // coordinates, and parent sectors from walls
    // that we just found

    for (nLoop = numloops, p = loop.GetPtr(), t = 0; *p >= 0; p++)
    {
        nSrc = *p;
        if (nSrc == kMaxWalls)
        {
            parentFound = 0;
            continue; // end of loop
        }

        pSrc = &wall[nSrc];
        pDst = &pWalls[t++];

        if (!parentFound)
        {
            nLoop--; // use reverse order for loops

            // searching the parent for z-offset sectors
            for (int32_t* p2 = p; *p2 != kMaxWalls; p2++)
            {
                if ((nNext = gNextWall[*p2]) >= 0)
                {
                    memcpy(&pParent[nLoop], &sector[sectorofwall(nNext)], sizeof(sectortype));
                    parentFound = 1;
                    break;
                }
            }

            // no luck!
            if (!parentFound)
            {
                memcpy(&pParent[nLoop], &sector[sectorofwall(nSrc)], sizeof(sectortype));
                pParent[nLoop].floorz = pParent[nLoop].ceilingz = 0;
                parentFound = 1;
            }
        }

        // it's ok to use source wall when there is no next
        pNext = (gNextWall[nSrc] >= 0) ? &wall[gNextWall[nSrc]] : pSrc;
        memcpy(pDst, pNext, sizeof(walltype));

        // source coords
        pDst->x = pSrc->x;
        pDst->y = pSrc->y;

        if (pNext == pSrc)
        {
            // clear these to avoid duplicates
            pDst->extra = -1;   pDst->cstat = 0;
            pDst->hitag =  0;   pDst->type  = 0;
        }
    }

    // Construct and insert loops
    for (nLoop = numloops, p = loop.GetPtr(); *p >= 0; p++)
    {
        zOffsetsDone = 0;
        nLoop--;
        c = 0;

        while(*p != kMaxWalls)
        {
            // constructing
            pPoint[c].x = pWalls[nWall].x;
            pPoint[c].y = pWalls[nWall].y;
            nWall++, c++;
            p++;
        }

        if (numwalls + c >= kMaxWalls)
            break;

        if ((nParent = ARW_helperLoopFindParentSect(pPoint, c, ignore)) < 0)
            continue;

        if ((nDst = insertLoop(nParent, pPoint, c)) < 0)
            continue;

        // Loop was inserted - attach walls to each other
        // and z-offset a whole thing relative to new
        // parent sector

        i = nDst;
        do
        {
            if ((nNext = ARW_helperFindNextWall(i)) >= 0)
            {
                if ((nSect = sectorofwall(nNext)) >= 0)
                {
                    if ((flags & 0x01) && !zOffsetsDone)
                    {
                        sectortype* pSect = &pParent[nLoop];
                        if (pSect->floorz && pSect->floorz != pSect->ceilingz)
                        {
                            // Here, unfortunately, we have to collect all the sectors
                            // with nextwalls inside this loop because we cannot
                            // rely on ignore list anymore

                            IDLIST list(true);
                            ARW_helperCollectSectors(nSect, &list);
                            for (int32_t* s = list.GetPtr(); *s >= 0; s++)
                                sectSetupZOffset(*s, &pParent[nLoop], &sector[nParent], 0x03);
                        }

                        // Done or skipped for this whole loop
                        zOffsetsDone = 1;
                    }

                    // Attach walls to each other
                    wallAttach(i,   nSect, nNext);  gNextWall[i] = nNext;
                    wallAttach(nNext, nParent, i);  gNextWall[nNext] = i;
                }
            }

            i = wall[i].point2;
        }
        while(i != nDst);


        // Finally, copy properties from old nextwalls to a new ones...
        // is not really a fast task because the correct wall order
        // is NOT guaranteed, so we have to compare coordinates to
        // make sure we copying to the right
        // destination

        for (t = nWall - c; t < nWall; t++)
        {
            pSrc = &pWalls[t];
            i = nDst;

            do
            {
                pDst = &wall[i];
                pNext = (pDst->nextwall >= 0) ? &wall[pDst->nextwall] : pDst;

                if (pNext->x == pSrc->x && pNext->y == pSrc->y)
                {
                    // Found matching wall! Safe to copy!

                    pDst->picnum        = pSrc->picnum;
                    pDst->overpicnum    = pSrc->overpicnum;
                    pDst->pal           = pSrc->pal;
                    pDst->shade         = pSrc->shade;

                    pDst->xpanning      = pSrc->xpanning;
                    pDst->ypanning      = pSrc->ypanning;
                    pDst->xrepeat       = pSrc->xrepeat;
                    pDst->yrepeat       = pSrc->yrepeat;

                    pDst->type          = pSrc->type;
                    pDst->extra         = pSrc->extra;
                    pDst->hitag         = pSrc->hitag;
                    pDst->cstat         = pSrc->cstat;

                    break;
                }

                i = wall[i].point2;
            }
            while(i != nDst);
        }

        nLoopsDone++;

    }

    free(pPoint), free(pWalls), free(pParent);
    return nLoopsDone;
}

unsigned char fixupPanCountShift(int nPicSiz)
{
    int nSiz = 16;
    int i = 0;

    while(i < 32)
    {
        if (nSiz == nPicSiz)
            return i;

        nSiz<<=1;
        i++;
    }

    return 0;
}

// !!! FIXME:
// Unfortunately, this code stops working when:
// picnum size is not power of two
// picnum size is less than 16 pixels
// dist step is less than 8 pixels
void fixupPan(int nSect, int nStat, int tx, int ty)
{
    sectortype* pSect = &sector[nSect];
    int nCstat = sectCstatGet(nSect, nStat);
    int nPic, x, y, t;

    char swap = ((nCstat & kSectSwapXY) > 0);
    char expd = ((nCstat & kSectExpand) > 0);
    unsigned char *pPanX, *pPanY;

    if (nStat == OBJ_FLOOR)
    {
        nPic  = pSect->floorpicnum;
        pPanX = &pSect->floorxpanning;
        pPanY = &pSect->floorypanning;
    }
    else
    {
        nPic  = pSect->ceilingpicnum;
        pPanX = &pSect->ceilingxpanning;
        pPanY = &pSect->ceilingypanning;
    }

    if (swap)
        swapValues(&tx, &ty);

    if (tx)
    {
        x = fixupPanCountShift(tilesizx[nPic])-expd;
        t = tx>>x;
        if (nCstat & kSectFlipX)
            t = revertValue(t);

        *pPanX += (swap) ? t : -t;
    }

    if (ty)
    {
        y = fixupPanCountShift(tilesizy[nPic])-expd;
        t = ty>>y;
        if (nCstat & kSectFlipY)
            t = revertValue(t);

        *pPanY += (swap) ? -t : t;
    }
}

void sectChgXY(int nSector, int bx, int by, int flags, int)
{
    int s, e;
    sectortype* pSect = &sector[nSector];
    getSectorWalls(nSector, &s, &e);

    if (flags & 0x01)
    {
        if (!(pSect->floorstat & kSectRelAlign) && !(pSect->floorstat & kSectParallax))
            fixupPan(nSector, OBJ_FLOOR, bx, by);

        if (!(pSect->ceilingstat & kSectRelAlign) && !(pSect->ceilingstat & kSectParallax))
            fixupPan(nSector, OBJ_CEILING, bx, by);
    }

    while(s <= e)
    {
        wall[s].x += bx;
        wall[s].y += by;
        s++;
    }

    if (flags & 0x02)
    {
        for (s = headspritesect[nSector]; s >= 0; s = nextspritesect[s])
        {
            sprite[s].x += bx;
            sprite[s].y += by;
        }
    }
}

void sectChgZ(int nSect, int bz, int a2, int flags, int a4)
{
    int s;
    sectortype* pSect = &sector[nSect];
    XSECTOR* pXSect = GetXSect(pSect);

    pSect->floorz       += bz;
    pSect->ceilingz     += bz;

    if (pXSect && (pXSect->onFloorZ || pXSect->offFloorZ))
    {
        pXSect->onFloorZ    += bz;
        pXSect->offFloorZ   += bz;
    }

    for (s = headspritesect[nSect]; s >= 0; s = nextspritesect[s])
        sprite[s].z += bz;

}

BOOL hgltShowStatus(int x, int y) {

    QFONT* pFont = qFonts[5];
    int xstep = pFont->width;
    int nLen = gListGrd.Length();


    if (nLen > 0 || highlightcnt > 0 || highlightsectorcnt > 0)
    {
        sprintf(buffer, "HIGHLIGHT:");
        gfxPrinTextShadow(x, y, gStdColor[kColorYellow], buffer, pFont);
        x += gfxGetTextLen(buffer, pFont) + xstep;

        if (nLen > 0)
        {
            sprintf(buffer, "GRADIENT=%d", nLen);
            gfxPrinTextShadow(x, y, gStdColor[kColorLightMagenta], buffer, pFont);
            x += gfxGetTextLen(buffer, pFont) + xstep;
        }

        if (highlightcnt > 0)
        {
            int sprites = hgltSprCount();
            if (sprites)
            {
                sprintf(buffer, "SPRITES=%d", sprites);
                gfxPrinTextShadow(x, y, gStdColor[kColorLightCyan], buffer, pFont);
                x += gfxGetTextLen(buffer, pFont) + xstep;
            }

            int walls = highlightcnt - sprites;
            if (walls > 0)
            {
                sprintf(buffer, "WALLS=%d", walls);
                gfxPrinTextShadow(x, y, gStdColor[kColorLightRed], buffer, pFont);
                x += gfxGetTextLen(buffer, pFont) + xstep;
            }
        }

        if (highlightsectorcnt > 0)
        {
            sprintf(buffer, "SECTORS=%d", highlightsectorcnt);
            gfxPrinTextShadow(x, y, gStdColor[kColorLightGreen], buffer, pFont);
            x += gfxGetTextLen(buffer, pFont) + xstep;
        }

        return TRUE;
    }

    return FALSE;

}

void sprPalSet(spritetype* pSprite, int value) {

    pSprite->pal = (char)ClipRange(value, 0, 255);

}

void sprShadeSet(spritetype* pSprite, int value) {

    setShadeOf(value, OBJ_SPRITE, pSprite->index);
}

void sprShadeIterate(spritetype* pSprite, int step) {

    iterShadeOf(step, OBJ_SPRITE, pSprite->index);
}

void sprClone(spritetype* pSprite, int) {

    int i;
    if ((i = InsertSprite(pSprite->sectnum, pSprite->statnum)) >= 0)
    {
        short sect = sprite[i].sectnum;
        memcpy(&sprite[i], pSprite, sizeof(spritetype));
        sprite[i].sectnum = sect;
        sprite[i].index = (short)i;

        ChangeSpriteSect((short)i, sect);

        if (sprInHglt(pSprite->index))
        {
            hgltRemove(OBJ_SPRITE, pSprite->index);
            hgltAdd(OBJ_SPRITE, sprite[i].index);
        }
    }
}

void sprDelete(spritetype* pSprite, int) {

    hgltRemove(OBJ_SPRITE, pSprite->index);
    if (pSprite->statnum < kMaxStatus)
        DeleteSprite(pSprite->index);

}

void sprSetXRepeat(spritetype* pSprite, int val) {

    pSprite->xrepeat += val;

}

void sprSetYRepeat(spritetype* pSprite, int val) {

    pSprite->yrepeat += val;

}

/* void sectFXChgFreq(int nSect, int nVal)
{
    int nXSect = sector[nSect].extra;
    if (nXSect > 0)
        xsector[nXSect].freq = ClipRange(xsector[nXSect].freq + nVal, 0, 255);
}

void sectFXChgPhase(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].phase = ClipRange(xsector[nXSect].phase + nVal, 0, 255);
}

void sectFXChgAmplitude(int nSect, int nVal)
{
    int nXSect = GetXSector(nSect);
    xsector[nXSect].amplitude = ClipRange(xsector[nXSect].amplitude + nVal, -128, 127);
} */

void sectChgVisibility(int nSect, int nVis)
{
    sector[nSect].visibility = ClipRange(sector[nSect].visibility + nVis, 0, 239);
}

void sectChgShade(int nSect, int nOf, int nShade, int, int)
{
    iterShadeOf(nShade, nOf, nSect);
}

void sectDelete(int nSector, int arg1, int arg2, int arg3, int arg4)
{
    int i;
    deletesector(nSector);
    if (highlightsectorcnt > 0)
    {
        // exclude this one from highlight, if any
        hgltRemove(OBJ_SECTOR, nSector);

        i = highlightsectorcnt;
        while(--i >= 0)
        {
            // must fix the highlight indexes
            if (highlightsector[i] >= nSector)
                highlightsector[i]--;
        }
    }
}

void sectSetupZOffset(int nSect, sectortype* pOParent, sectortype* pNParent, int flags)
{
    sectortype* pSect    = &sector[nSect];
    XSECTOR* pXSect      = GetXSect(pSect);

    int nFOffs = 0, nCOffs = 0;
    int z1, z2, z3, i;

    if (flags & 0x01)
    {
        z1  = pOParent->floorz;
        z2  = pNParent->floorz;
        z3  = pSect->floorz;

        nFOffs = (z3-z2) + (z1-z3);
        if (z2 < 0)
        {
            nFOffs = (z2 > z1) ? klabs(nFOffs) : -nFOffs;
        }
        else
        {
            nFOffs = (z2 < z1) ? -nFOffs : klabs(nFOffs);
        }

        if (pXSect)
        {
            pXSect->offFloorZ += nFOffs;
            pXSect->onFloorZ += nFOffs;
        }

        pSect->floorz += nFOffs;
        if (pSect->floorz < pNParent->ceilingz)
            pSect->ceilingz -= (pNParent->ceilingz - pSect->floorz);

        // offset relative to floor
        for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
        {
            spritetype* pSpr = &sprite[i];
            pSpr->z += nFOffs;
        }

    }
    else if (flags & 0x04)
    {
        pSect->floorz           = pNParent->floorz;
        pSect->floorslope       = pNParent->floorslope;
        pSect->floorpicnum      = pNParent->floorpicnum;
        pSect->floorshade       = pNParent->floorshade;
        pSect->floorstat        = pNParent->floorstat;
        pSect->floorpal         = pNParent->floorpal;

        pSect->floorxpanning    = pNParent->floorxpanning;
        pSect->floorypanning    = pNParent->floorypanning;
    }

    if (flags & 0x02)
    {
        z1 = pOParent->ceilingz;
        z2 = pNParent->ceilingz;
        z3 = pSect->ceilingz;

        nCOffs = (z3-z2) + (z1-z3);
        if (z2 < 0)
        {
            nCOffs = (z2 > z1) ? klabs(nCOffs) : -nCOffs;
        }
        else
        {
            nCOffs = (z2 < z1) ? -nCOffs : klabs(nCOffs);
        }

        if (pXSect)
        {
            pXSect->offCeilZ += nCOffs;
            pXSect->onCeilZ += nCOffs;
        }

        pSect->ceilingz += nCOffs;
        if (pSect->ceilingz > pNParent->floorz)
            pSect->floorz -= (pNParent->floorz - pSect->ceilingz);
    }
    else if (flags & 0x08)
    {
        pSect->ceilingz         = pNParent->ceilingz;
        pSect->ceilingslope     = pNParent->ceilingslope;
        pSect->ceilingpicnum    = pNParent->ceilingpicnum;
        pSect->ceilingshade     = pNParent->ceilingshade;
        pSect->ceilingstat      = pNParent->ceilingstat;
        pSect->ceilingpal       = pNParent->ceilingpal;

        pSect->ceilingxpanning  = pNParent->ceilingxpanning;
        pSect->ceilingypanning  = pNParent->ceilingypanning;
    }
}


void sectSetupZOffset(int nSect, int nParentOld, int nParentNew, int flags, int)
{
    sectSetupZOffset(nSect, &sector[nParentOld], &sector[nParentNew], flags);
}

static int ARW_helperLoopFindParentSect(POINT2D* pPoint, int c, BITSECTOR visited)
{
    static int16_t* score = NULL; static int nSiz = 0;
    int nParent = -1, t = sizeof(int16_t)*numsectors;
    int n = 0, i;

    // find a large enough sector for loop
    // (intersection allowed)

    if (nSiz < t)
        nSiz = t, score = (int16_t*)realloc(score, t);

    if (score != NULL)
    {
        memset(score, 0, nSiz);

        t = numsectors;
        while(--t >= 0)
        {
            if (!TestBitString(visited, t))
            {
                for (i = 0; i < c; i++)
                {
                    if (inside(pPoint[i].x, pPoint[i].y, t))
                        score[t]++;
                }

                if (score[t] == c)
                    return t;
            }
        }

        t = numsectors;
        while(--t >= 0)
        {
            if (score[t] > n)
                n = score[t], nParent = t;
        }
    }

    return nParent;

}

static void ARW_helperCollectSectors(int nStart, IDLIST* pList)
{
    int s, e, n;

    // reculsively collect all the sectors with two-sided walls
    // while not separated by
    // one-sided walls

    getSectorWalls(nStart, &s, &e);
    pList->Add(nStart);

    while(s <= e)
    {
        if ((n = wall[s].nextsector) >= 0 && !pList->Exists(n))
            ARW_helperCollectSectors(n, pList);

        s++;
    }
}

static void ARW_helperMarkVisitedInside(int nStart, BITSECTOR visited)
{
    int s, e, n;

    // reculsively mark all the sectors with two-sided walls
    // while not separated by
    // one-sided walls

    getSectorWalls(nStart, &s, &e);
    SetBitString(visited, nStart);

    while(s <= e)
    {
        if ((n = wall[s].nextsector) >= 0 && !TestBitString(visited, n))
            ARW_helperMarkVisitedInside(n, visited);

        s++;
    }
}

static int ARW_helperFindNextWall(int nWall)
{
    int s, e, i = highlightsectorcnt;

    // version of findNextWall() limited to
    // highlighsector array scope
    // only

    while(--i >= 0)
    {
        getSectorWalls(highlightsector[i], &s, &e);
        while(s <= e)
        {
            if (isNextWallOf(nWall, s))
                return s;

            s++;
        }
    }

    return -1;
}
