/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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
#include "editor.h"
#include "compat.h"
#include "common_game.h"
#include "crc32.h"
#include "db.h"
#include "iob.h"
#include "misc.h"

#include "mapcmt.h"
#include "xmpconf.h"
#include "xmpstub.h"
#include "xmpmisc.h"


unsigned short gStatCount[kMaxStatus + 1];
int gSuppBuildMapVersion[] = {
	4,
	6,
	7,
	//8,	// same as 7, but with higher limits?
	//9,	// ion fury
};

XSPRITE xsprite[kMaxXSprites];
XSECTOR xsector[kMaxXSectors];
XWALL xwall[kMaxXWalls];

SPRITEHIT gSpriteHit[kMaxXSprites];

bool gModernMap = false, gCustomSkyBits = false;
bool gByte1A76C6 = false, gByte1A76C7 = false, gByte1A76C8 = false;
short numxsprites = 0, numxwalls = 0, numxsectors = 0;
const int nXSectorSize = 60, nXSpriteSize = 56, nXWallSize = 24;
int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];
int gMapRev, gSongId, gSkyCount;
MAPHEADER2 mapHead;

#pragma pack(push,1)
struct SECTOR4 {
	uint16_t wallptr, wallnum;
	int8_t   ceilingstat, ceilingxpanning, ceilingypanning;
	int8_t   ceilingshade;
	int32_t  ceilingz;
	int16_t  ceilingpicnum, ceilingheinum;
	int8_t   floorstat, floorxpanning, floorypanning;
	int8_t   floorshade;
	int32_t  floorz;
	int16_t  floorpicnum, floorheinum;
	int32_t  tag;
};

struct WALL4 {
	int32_t x, y;
	int16_t point2;
	int8_t cstat;
	int8_t shade;
	uint8_t xrepeat, yrepeat, xpanning, ypanning;
	int16_t picnum, overpicnum;
	int16_t nextsector1, nextwall1;
	int16_t nextsector2, nextwall2;
	int32_t tag;
};

struct SPRITE4 {
	int32_t x, y, z;
	int8_t cstat, shade;
	uint8_t xrepeat, yrepeat;
	int16_t picnum, ang, xvel, yvel, zvel, owner;
	int16_t sectnum, statnum;
	int32_t tag;
	int8_t *extra;
};

struct SECTOR6 {
	uint16_t wallptr, wallnum;
	int16_t ceilingpicnum, floorpicnum;
	int16_t ceilingheinum, floorheinum;
	int32_t ceilingz, floorz;
	int8_t ceilingshade, floorshade;
	uint8_t ceilingxpanning, floorxpanning;
	uint8_t ceilingypanning, floorypanning;
	uint8_t ceilingstat, floorstat;
	uint8_t ceilingpal, floorpal;
	uint8_t visibility;
	int16_t lotag, hitag, extra;
};

struct WALL6 {
	int32_t x, y;
	int16_t point2, nextsector, nextwall;
	int16_t picnum, overpicnum;
	int8_t  shade;
	int8_t  pal;
	int16_t cstat;
	int8_t xrepeat, yrepeat, xpanning, ypanning;
	int16_t lotag, hitag, extra;
};

struct SPRITE6 {
	int32_t x, y, z;
	int16_t cstat;
	int8_t  shade;
	uint8_t pal, clipdist;
	uint8_t xrepeat, yrepeat;
	int8_t  xoffset, yoffset;
	int16_t picnum, ang, xvel, yvel, zvel, owner;
	int16_t sectnum, statnum;
	int16_t lotag, hitag, extra;
};
#pragma pack(pop)

void dbCrypt(char *pPtr, int nLength, int nKey)
{
    for (int i = 0; i < nLength; i++)
    {
        pPtr[i] = pPtr[i] ^ nKey;
        nKey++;
    }
}

void InsertSpriteSect(int nSprite, int nSector)
{

	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	dassert(nSector >= 0 && nSector < kMaxSectors);

    int nOther = headspritesect[nSector];
    if (nOther >= 0)
    {
        prevspritesect[nSprite] = prevspritesect[nOther];
        nextspritesect[nSprite] = -1;
        nextspritesect[prevspritesect[nOther]] = nSprite;
        prevspritesect[nOther] = nSprite;
    }
    else
    {
        prevspritesect[nSprite] = nSprite;
        nextspritesect[nSprite] = -1;
        headspritesect[nSector] = nSprite;
    }
    sprite[nSprite].sectnum = nSector;
}

void RemoveSpriteSect(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    int nSector = sprite[nSprite].sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nOther = nextspritesect[nSprite];
    if (nOther < 0)
    {
        nOther = headspritesect[nSector];
    }
    prevspritesect[nOther] = prevspritesect[nSprite];
    if (headspritesect[nSector] != nSprite)
    {
        nextspritesect[prevspritesect[nSprite]] = nextspritesect[nSprite];
    }
    else
    {
        headspritesect[nSector] = nextspritesect[nSprite];
    }
    sprite[nSprite].sectnum = -1;
}

void InsertSpriteStat(int nSprite, int nStat)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = headspritestat[nStat];
    if (nOther >= 0)
    {
        prevspritestat[nSprite] = prevspritestat[nOther];
        nextspritestat[nSprite] = -1;
        nextspritestat[prevspritestat[nOther]] = nSprite;
        prevspritestat[nOther] = nSprite;
    }
    else
    {
        prevspritestat[nSprite] = nSprite;
        nextspritestat[nSprite] = -1;
        headspritestat[nStat] = nSprite;
    }
    sprite[nSprite].statnum = nStat;
    gStatCount[nStat]++;
}

void RemoveSpriteStat(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    int nStat = sprite[nSprite].statnum;
    dassert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = nextspritestat[nSprite];
    if (nOther < 0)
    {
        nOther = headspritestat[nStat];
    }
    prevspritestat[nOther] = prevspritestat[nSprite];
    if (headspritestat[nStat] != nSprite)
    {
        nextspritestat[prevspritestat[nSprite]] = nextspritestat[nSprite];
    }
    else
    {
        headspritestat[nStat] = nextspritestat[nSprite];
    }
    sprite[nSprite].statnum = kStatNothing;
    gStatCount[nStat]--;
}

void qinitspritelists(void) // Replace
{
    for (short i = 0; i <= kMaxSectors; i++)
    {
        headspritesect[i] = -1;
    }
    for (short i = 0; i <= kMaxStatus; i++)
    {
        headspritestat[i] = -1;
    }

    for (short i = 0; i < kMaxSprites; i++)
    {
        sprite[i].sectnum = -1;
        sprite[i].index = -1;
        InsertSpriteStat(i, kMaxStatus);
    }
    memset(gStatCount, 0, sizeof(gStatCount));
    // Numsprites = 0;
}

int InsertSprite(int nSector, int nStat)
{
    int nSprite = headspritestat[kMaxStatus];
    dassert(nSprite < kMaxSprites);
    if (nSprite < 0)
    {
        return nSprite;
    }
    RemoveSpriteStat(nSprite);
    spritetype *pSprite = &sprite[nSprite];
    memset(&sprite[nSprite], 0, sizeof(spritetype));
    InsertSpriteStat(nSprite, nStat);
    InsertSpriteSect(nSprite, nSector);
    pSprite->cstat = defaultspritecstat;
    pSprite->clipdist = 32;
    pSprite->xrepeat = pSprite->yrepeat = 64;
    pSprite->owner = -1;
    pSprite->extra = -1;
    pSprite->index = nSprite;

	xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
	numsprites++;

    return nSprite;
}

int qinsertsprite(short nSector, short nStat) // Replace
{
    return InsertSprite(nSector, nStat);
}

int DeleteSprite(int nSprite)
{
    if (sprite[nSprite].extra > 0)
        dbDeleteXSprite(sprite[nSprite].extra);

    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    RemoveSpriteStat(nSprite);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteSect(nSprite);
    InsertSpriteStat(nSprite, kMaxStatus);

    numsprites--;

    return nSprite;
}



int ChangeSpriteSect(int nSprite, int nSector)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nSector >= 0 && nSector < kMaxSectors);
    //dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);

	// is error occured on BUILD side or on DB side?
	if (!(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus))
		ThrowError("DB STATCHECK !!! [%d]: nSprite=%d .statnum=%d .sectnum=%d nSector=%d .index=%d", highlightcnt, nSprite, sprite[nSprite].statnum, sprite[nSprite].sectnum, nSector, sprite[nSprite].index);

	// is error occured on BUILD side or on DB side?
	if (!(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors))
		ThrowError("DB SECTCHECK !!! [%d]: nSprite=%d .statnum=%d .sectnum=%d nSector=%d .index=%d", highlightcnt, nSprite, sprite[nSprite].statnum, sprite[nSprite].sectnum, nSector, sprite[nSprite].index);

    RemoveSpriteSect(nSprite);
    InsertSpriteSect(nSprite, nSector);
    return 0;
}


int ChangeSpriteStat(int nSprite, int nStatus)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nStatus >= 0 && nStatus < kMaxStatus);
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteStat(nSprite);
    InsertSpriteStat(nSprite, nStatus);
    return 0;
}
int qdeletesprite(short nSprite) { return DeleteSprite(nSprite); }
int qchangespritesect(short nSprite, short nSector) { return ChangeSpriteSect(nSprite, nSector); }
int qchangespritestat(short nSprite, short nStatus) { return ChangeSpriteStat(nSprite, nStatus); }

unsigned short nextXSprite[kMaxXSprites];
unsigned short nextXWall[kMaxXWalls];
unsigned short nextXSector[kMaxXSectors];

void InitFreeList(unsigned short *pList, int nCount)
{
	// make free EXTRAs to be assigned from start of array rather end
	// this makes FS like ports to live with extended limits
	for (int i = 0; i < nCount; i++) pList[i] = i + 1;
	pList[0] = 1;
}

void InsertFree(unsigned short *pList, int nIndex)
{
    pList[nIndex] = pList[0];
    pList[0] = nIndex;
}

unsigned short dbInsertXSprite(int nSprite)
{
    int nXSprite = nextXSprite[0];
    nextXSprite[0] = nextXSprite[nXSprite];
    if (nXSprite == 0)
        ThrowError("Out of free XSprites");

    memset(&xsprite[nXSprite], 0, sizeof(XSPRITE));
    xsprite[nXSprite].reference = nSprite;
    sprite[nSprite].extra = nXSprite;
	numxsprites++;

    return nXSprite;
}

void dbDeleteXSprite(int nXSprite)
{
    dassert(xsprite[nXSprite].reference >= 0);
    dassert(sprite[xsprite[nXSprite].reference].extra == nXSprite);
    InsertFree(nextXSprite, nXSprite);
    sprite[xsprite[nXSprite].reference].extra = -1;
    xsprite[nXSprite].reference = -1;
	numxsprites--;
}

unsigned short dbInsertXWall(int nWall)
{
    int nXWall = nextXWall[0];
    nextXWall[0] = nextXWall[nXWall];
    if (nXWall == 0)
        ThrowError("Out of free XWalls");

    memset(&xwall[nXWall], 0, sizeof(XWALL));
    xwall[nXWall].reference = nWall;
    wall[nWall].extra = nXWall;

	numxwalls++;
    return nXWall;
}

void dbDeleteXWall(int nXWall)
{
    dassert(xwall[nXWall].reference >= 0);
    InsertFree(nextXWall, nXWall);
    wall[xwall[nXWall].reference].extra = -1;
    xwall[nXWall].reference = -1;
	numxwalls--;
}

unsigned short dbInsertXSector(int nSector)
{
    int nXSector = nextXSector[0];
    nextXSector[0] = nextXSector[nXSector];
    if (nXSector == 0)
        ThrowError("Out of free XSectors");

    memset(&xsector[nXSector], 0, sizeof(XSECTOR));
    xsector[nXSector].reference = nSector;
    sector[nSector].extra = nXSector;

	numxsectors++;
    return nXSector;
}

void dbDeleteXSector(int nXSector)
{
    dassert(xsector[nXSector].reference >= 0);
    InsertFree(nextXSector, nXSector);
    sector[xsector[nXSector].reference].extra = -1;
    xsector[nXSector].reference = -1;
	numxsectors--;
}

void dbXSpriteClean(void)
{
    for (int i = 0; i < kMaxSprites; i++)
    {
        int nXSprite = sprite[i].extra;
        if (nXSprite == 0)
        {
            sprite[i].extra = -1;
        }
        if (sprite[i].statnum < kMaxStatus && nXSprite > 0)
        {
            dassert(nXSprite < kMaxXSprites);
            if (xsprite[nXSprite].reference != i)
            {
                int nXSprite2 = dbInsertXSprite(i);
                memcpy(&xsprite[nXSprite2], &xsprite[nXSprite], sizeof(XSPRITE));
                xsprite[nXSprite2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXSprites; i++)
    {
        int nSprite = xsprite[i].reference;
        if (nSprite >= 0)
        {
            dassert(nSprite < kMaxSprites);
            if (sprite[nSprite].statnum >= kMaxStatus || sprite[nSprite].extra != i)
            {
                InsertFree(nextXSprite, i);
                xsprite[i].reference = -1;
            }
        }
    }
}

void dbXWallClean(void)
{
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall == 0)
        {
            wall[i].extra = -1;
        }
        if (nXWall > 0)
        {
            dassert(nXWall < kMaxXWalls);
            if (xwall[nXWall].reference == -1)
            {
                wall[i].extra = -1;
            }
            else
            {
                xwall[nXWall].reference = i;
            }
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall > 0)
        {
            dassert(nXWall < kMaxXWalls);
            if (xwall[nXWall].reference != i)
            {
                int nXWall2 = dbInsertXWall(i);
                memcpy(&xwall[nXWall2], &xwall[nXWall], sizeof(XWALL));
                xwall[nXWall2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXWalls; i++)
    {
        int nWall = xwall[i].reference;
        if (nWall >= 0)
        {
            dassert(nWall < kMaxWalls);
            if (nWall >= numwalls || wall[nWall].extra != i)
            {
                InsertFree(nextXWall, i);
                xwall[i].reference = -1;
            }
        }
    }
}

void dbXSectorClean(void)
{


    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector == 0)
        {
            sector[i].extra = -1;
        }
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            if (xsector[nXSector].reference == -1)
            {
                sector[i].extra = -1;
            }
            else
            {
                xsector[nXSector].reference = i;
            }
        }
    }
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            if (xsector[nXSector].reference != i)
            {
                int nXSector2 = dbInsertXSector(i);
                memcpy(&xsector[nXSector2], &xsector[nXSector], sizeof(XSECTOR));
                xsector[nXSector2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXSectors; i++)
    {
        int nSector = xsector[i].reference;
        if (nSector >= 0)
        {
            dassert(nSector < kMaxSectors);
            if (nSector >= numsectors || sector[nSector].extra != i)
            {
                InsertFree(nextXSector, i);
                xsector[i].reference = -1;
            }
        }
    }
}

void dbInit(void)
{
    InitFreeList(nextXSprite, kMaxXSprites);
    for (int i = 1; i < kMaxXSprites; i++)
    {
        xsprite[i].reference = -1;
    }
    InitFreeList(nextXWall, kMaxXWalls);
    for (int i = 1; i < kMaxXWalls; i++)
    {
        xwall[i].reference = -1;
    }
    InitFreeList(nextXSector, kMaxXSectors);
    for (int i = 1; i < kMaxXSectors; i++)
    {
        xsector[i].reference = -1;
    }
    initspritelists();
    for (int i = 0; i < kMaxSprites; i++)
    {
        sprite[i].cstat = defaultspritecstat;
    }
}

void PropagateMarkerReferences(void)
{
    int nSprite, nNextSprite;
    for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = nNextSprite) {

        nNextSprite = nextspritestat[nSprite];

        switch (sprite[nSprite].type)  {
            case kMarkerOff:
            case kMarkerAxis:
            case kMarkerWarpDest: {
                int nOwner = sprite[nSprite].owner;
                if (nOwner >= 0 && nOwner < numsectors) {
                    int nXSector = sector[nOwner].extra;
                    if (nXSector > 0 && nXSector < kMaxXSectors) {
                        xsector[nXSector].marker0 = nSprite;
                        continue;
                    }
                }
            }
            break;
            case kMarkerOn: {
                int nOwner = sprite[nSprite].owner;
                if (nOwner >= 0 && nOwner < numsectors) {
                    int nXSector = sector[nOwner].extra;
                    if (nXSector > 0 && nXSector < kMaxXSectors) {
                        xsector[nXSector].marker1 = nSprite;
                        continue;
                    }
                }
            }
            break;
        }

        DeleteSprite(nSprite);
    }
}

#if 0
XWALL*  GetPXWall(int nWall) {
	
	return &xwall[GetXWall(nWall)];
}

XSECTOR* GetPXSector(int nSector) {

	return &xsector[GetXSector(nSector)];
}

XSPRITE* GetPXSprite(int nSprite) {
	
	return &xsprite[GetXSprite(nSprite)];
	
}

int nextFZA(int nSect) {
	
	nSect = nextsectorneighborz(nSect, sector[nSect].floorz, 1, -1);
	dassert(nSect >= 0);
	
	return sector[nSect].floorz;
}

int nextFZB(int nSect) {
	
	nSect = nextsectorneighborz(nSect, sector[nSect].floorz, 1, 1);
	dassert(nSect >= 0);
	
	return sector[nSect].floorz;
}



int nextCZA(int nSect) {
	
	nSect = nextsectorneighborz(nSect, sector[nSect].ceilingz, -1, -1);
	dassert(nSect >= 0);
	
	return sector[nSect].ceilingz;
}

int nextCZA2(int nSect, int ofs) {
	
	nSect = nextsectorneighborz(nSect, sector[nSect].floorz + ofs, -1, -1);
	dassert(nSect >= 0);
	
	return sector[nSect].ceilingz;
	
}

//void wayPoint2Path(spritetype* pSpr)

void wayPointInc(int nHitag)
{
	spritetype* pSpr; int i;
	
	for (i = 0; i < kMaxSprites; i++)
	{
		pSpr =& sprite[i];
		if (pSpr->statnum >= kMaxStatus) continue;
		else if (pSpr->type >= 900  && pSpr->type < 950 && pSpr->hitag == nHitag)
			pSpr->type++;
	}

	return;
}

int findFirstWayPoint(int nSector) {
	
	spritetype* pSpr; int i;
	
	for (i = 0; i < kMaxSprites; i++)
	{
		pSpr =& sprite[i];
		if (pSpr->statnum >= kMaxStatus) continue;
		else if (pSpr->type == 900 && pSpr->hitag == sector[nSector].hitag)
			return pSpr->index;
	}

	return -1;
}

int findNextWayPoint(int nHitag, int nId, int nDir = 1) {
	
	spritetype* pSpr; int i;
	for (i = 0; i < kMaxSprites; i++)
	{
		pSpr =& sprite[i];
		if (pSpr->statnum >= kMaxStatus) continue;
		else if (pSpr->type < 900 || pSpr->type >= 950 || pSpr->hitag != nHitag) continue;
		else if (nDir > 0 && nId+1 == pSpr->type) return pSpr->index;
		else if (nDir < 0 && nId-1 == pSpr->type) return pSpr->index;
		
	}
	
	return -1;
}

spritetype* insertSprite(int nSect, int x, int y, int z, XSPRITE** pXSpr = NULL) {
	
	int i;
	i = InsertSprite(nSect, 0);
	dassert(i >= 0);
			
	sprite[i].x = x;
	sprite[i].y = y;
	sprite[i].z = z;
	
	ChangeSpriteSect(i, nSect);
	
	if (pXSpr)
		*pXSpr = GetPXSprite(i);
	
	return &sprite[i];
}

int insertSwitch(int nSect, int x, int y, int z, int RX, int TX, int cmd) {
	
	int i, j; spritetype* pSpr; XSPRITE* pXSpr;
	pSpr = insertSprite(nSect, x, y, z, &pXSpr);
	pSpr->type = 20;
	
	pXSpr->triggerOn = pXSpr->triggerOff = 0;
	pXSpr->command = cmd;
	pXSpr->rxID = RX;
	pXSpr->txID = TX;
	
	return pSpr->index;
}

int insertEarthquake(int nSect, int x, int y, int z, int strength) {
		
	int i, j; spritetype* pSpr; XSPRITE* pXSpr;
	pSpr = insertSprite(nSect, x, y, z, &pXSpr);
	pXSpr->data1	= strength;
	pSpr->type		= kMarkerEarthquake;
	
	if ((j = adjIdxByType(pSpr->type)) >= 0)
		adjSetApperance(pSpr, j);
		
	clampSprite(pSpr);
	return pSpr->index;
}

// experimental conversion of triggers and stuff from Exhumed to Blood Xsystem
int exhum2xsystem() {
	
	int i, j, k, f, g, swal, ewal;
	
	
	spritetype *spr1, *spr2;	XSPRITE *xspr1, *xspr2;
	sectortype *sec1, *sec2; 	XSECTOR *xsec1, *xsec2;
	walltype *wal1, *wal2; 		XWALL *xwal1, *xwal2; 
	int spr1lo, spr2lo, spr1hi, spr2hi;
	int sec1lo, sec2lo, sec1hi, sec2hi;
	int wal1lo, wal2lo, wal1hi, wal2hi;
	
	int randBomb[] = {kItemAmmoTNTBundle, kItemAmmoRemoteBombBundle, kItemAmmoProxBombBundle};
	int inserted[kMaxSprites];
	
	memset(inserted, -1, sizeof(inserted));
	//int keymask[] = {1000, 2000, 4000, 8000}
	for (i = 0; i < kMaxSprites; i++)
	{
		if (inserted[i] == 1 || sprite[i].statnum == kMaxStatus) continue;
		spr1 =& sprite[i]; sec1 =& sector[spr1->sectnum];
		spr1lo = spr1->type % 1000; spr1hi = spr1->hitag;
				
		switch(spr1lo) {
/* 			case 45:
			//case 46:
			//case 47:
			//case 48:
				spr1->type = kItemKeySkull;
				for (j = 0; j < numsectors; j++)
				{
					if (sector[j].hitag & 1000)
					{
						xsec1 = GetPXSector(j);
						xsec1->key = kItemKeySkull - kItemKeyBase + 1;
					}
				}
				spr1->hitag = 0;
				break; */
			case 25:
				spr1->type = kItemArmorSuper;
				break;
			case 59:
				spr1->type = kItemArmorBasic;
				break;
			case 60:
				spr1->type = kSwitchOneWay;
				spr1->picnum = 318;
				xspr1 = GetPXSprite(spr1->index);
				xspr1->command = 64;
				xspr1->triggerOn = 1;
				xspr1->triggerPush = 1;
				xspr1->txID = 4;
				break;
			case 75:
				spr1->type = kThingObjectExplode;
				xspr1 = GetPXSprite(spr1->index);
				xspr1->data1 = xspr1->data2 = 20;
				xspr1->data4 = 313;
				break;
			case 76:
				spr1->type = kThingTNTBarrel;
				PutSpriteOnFloor(spr1, 0);
				break;
			case 20:
				spr1->type = kItemTwoGuns;
				break;
			case 6:
				spr1->type = (Chance(0x3000)) ? kItemAmmoFlares : kItemAmmoTommygunFew;
				break;
			case 7:
				spr1->type = kItemAmmoSprayCan;
				xspr1 = GetPXSprite(spr1->index);
				xspr1->data4 = spr1->hitag;
				break;
			case 8:
				spr1->type = kItemAmmoTommygunDrum;
				break;
			case 29:
				spr1->type = kItemWeaponTommygun;
				break;
			case 27:
			case 9:
				spr1->type = randBomb[Random(LENGTH(randBomb) - 1)];
				break;
			case 23:
				spr1->type = kItemDivingSuit;
				break;
			//case 10:
			//case 11:
				//break;
			case 12:
			case 17:
				spr1->type = 0;
				ChangeSpriteStat(spr1->index, 0);
				break;
			case 49:
				DeleteSprite(spr1->index);
				break;
			case 13:
				spr1->type = kItemHealthRedPotion;
				ChangeSpriteStat(spr1->index, kStatItem);
				break;
			// bubble gen
			case 93:
				spr1->type = kGenBubbleMulti;
				xspr1 = GetPXSprite(spr1->index);
				xspr1->busyTime	= ClipLow(2, Random(25));
				xspr1->data1 = ClipLow(2, BiRandom(25));
				xspr1->state = 1;
				spr1->cstat |= kSprInvisible;
				break;
			// sector underwater
			case 80:
				xsec1 = GetPXSector(spr1->sectnum);
				xsec1->underwater = 1;
				DeleteSprite(spr1->index);
				break;
			// sector panning angle
			case 88:
				xsec1 = GetPXSector(spr1->sectnum);
				xsec1->panAngle	= spr1->ang & kAngMask;
				DeleteSprite(spr1->index);
				break;
			// sector panning
			case 79:
			case 89:
				xsec1 = GetPXSector(spr1->sectnum);
				xsec1->panFloor		= (spr1lo == 89);
				xsec1->panCeiling	= (spr1lo == 79);
				xsec1->drag			= (spr1lo == 89);
				xsec1->panVel		= ClipLow((spr1->type / 1000), 1)<<2;
				xsec1->panAlways	= 1;
				DeleteSprite(spr1->index);
				break;
			// lower stack or water
			// if we found matching 99 here, it's gonna be a water stacks
			// if just 98, the it's gonna be normal stacks and the lower one must be created.
			case 98:
				f = spr1hi;
				for (j = headspritesect[f]; j >= 0; j = nextspritesect[j])
				{
					spr2 = &sprite[j]; spr2lo = spr2->type % 1000;
					if (spr2lo != 99 || spr2->hitag != spr1->sectnum) continue;
					spr2->type = kMarkerLowWater;
					xspr2 = GetPXSprite(spr2->index);
					xspr2->data1 = f;
					spr2->hitag = 0;
					
					xsec2 = GetPXSector(spr2->sectnum);
					xsec2->underwater = 1;
					break;
				}
							
				xspr1 = GetPXSprite(spr1->index);
				spr1->hitag = 0;
				xspr1->data1 = f;
				
				if (j >= 0)
				{
					spr1->type = kMarkerUpWater;
					spr2->x = spr1->x;
					spr2->y = spr1->y;

				}
				else if ((k = InsertSprite(f, 0)) >= 0)
				{
					spr2 = &sprite[k];
					spr2->x = spr1->x;
					spr2->y = spr1->y;
					spr2->type = kMarkerLowStack;
					
					ChangeSpriteSect(spr2->index, f);
					xspr2 = GetPXSprite(spr2->index);
					xspr2->data1 = f;
					
					spr1->type = kMarkerUpStack;
					inserted[k] = 1;
				}
				else
				{
					DeleteSprite(spr1->index);
				}
				break;
			case 94:
				xsec1 = GetPXSector(spr1->sectnum);
				xsec1->Depth = spr1hi & 7;
				DeleteSprite(spr1->index);
				break;
			// sector bobbing
			case 95:
			case 97:
				xsec1 = GetPXSector(spr1->sectnum);
				xsec1->bobZRange = 5;
				xsec1->bobSpeed = 30;
				xsec1->bobAlways = 1;
				if (spr1lo == 95)
					xsec1->bobFloor = 1;
				else
					xsec1->bobCeiling = 1;
				DeleteSprite(spr1->index);
				break;
			case 100:
				spr1->type = kDudeCultistShotgun;
				break;
			case 101:
				spr1->type = kDudeZombieButcher;
				break;
			case 102:
				spr1->type = (Chance(0x3000)) ? kDudeZombieAxeLaying  : kDudeZombieAxeNormal;
				break;
			case 105:
				spr1->type = (Chance(0x3000)) ? kDudeSpiderRed : (Chance(0x0100)) ? kDudeSpiderBlack : kDudeSpiderBrown;
				break;
			case 106:
				spr1->type = (Chance(0x3000)) ? kDudeGillBeast : kDudeBoneEel;
				break;
			case 115:
				spr1->type = kDudeRat; spr1->pal = 14;
				break;
			//case 900:
				//xsec1 = GetPXSector(spr1->sectnum);
				//xsec1->type = kSectorSlide;
				//break;
			// sector glow / flicker
			case 998:
			case 999:
				xsec1 = GetPXSector(spr1->sectnum);
				if (spr1lo == 998)
				{
					xsec1->shadeWave = 2;
					xsec1->amplitude = +10; // dunno how to count proper shade yet
				}
				else
				{
					xsec1->shadeWave = 7;
					xsec1->amplitude = -5; // dunno how to count proper shade yet
				}
				xsec1->shadeFreq = spr1->type / 1000;
				xsec1->shadeAlways	= 1;
				xsec1->shadeWalls	= 1;
				xsec1->shadeFloor	= 1;
				xsec1->shadeCeiling = 1;
				DeleteSprite(spr1->index);
				break;
			case 71:
				xspr1 = GetPXSprite(i);
				spr1->type = kGenMissileFireball;
				break;
		}
		
	}
	
	int avex, avey, avez, sec1vel;
	int swal2, ewal2;
	
	for (i = 0; i < numsectors; i++)
	{
		sec1 =& sector[i];
		sec1lo = sec1->type % 1000; sec1vel = ClipLow(1, sec1->type / 1000); //sec1vel <<= 2;
		sec1hi = sec1->hitag;
		
		switch (sec1lo) {
			case 1:
			case 5:
			case 6:
			case 8:
			case 10:
			case 11:
			case 12:
			case 33:
			case 34:
			case 56:
			case 58:
				xsec1 = GetPXSector(i);
				xsec1->busyWaveA = xsec1->busyWaveB = 1;
				switch(sec1lo) {
					case 1: // door (ceil)
					case 58:
					case 56:
						switch (sec1lo) {
							case 1:
								xsec1->interruptable	= 1;
								xsec1->waitTimeA		= 15;
								xsec1->reTriggerA		= 1;
								xsec1->triggerWallPush	= 1;
								break;
							case 58:
								xsec1->interruptable	= 1;
								xsec1->waitTimeA		= 15;
								xsec1->reTriggerA		= 1;
								break;
						}
						xsec1->offCeilZ  = sec1->ceilingz;
						xsec1->onCeilZ = nextCZA(i);
						xsec1->busyTimeA	= (abs(xsec1->offCeilZ - xsec1->onCeilZ) >> 10) & 255; // dunno
						xsec1->busyTimeB	= xsec1->busyTimeA;
						break;
					case 5: // floor raise (once)
					//case 6: // floor lower (multiple)
					case 8:	// floor lower (once)
						xsec1->offFloorZ	= sec1->floorz;
						xsec1->onFloorZ		= (sec1lo == 8) ? nextFZB(i) : nextCZA2(i, 1);
						xsec1->busyTimeA	= (abs(xsec1->offFloorZ - xsec1->onFloorZ) >> 10) & 255; // dunno
						break;
					case 10: // elevator
					case 11: // with waitTime
					case 12: // with waitTime
						xsec1->triggerEnter	= 1;
						xsec1->triggerExit	= 1;
						xsec1->offFloorZ	= sec1->floorz;
						xsec1->onFloorZ 	= nextFZA(i);
						xsec1->busyTimeA 	= xsec1->busyTimeB = sec1vel & 255;
						break;
					case 33: // ceil crusher (perman)
					case 34: // ceil crusher (by trigger)
						xsec1->offCeilZ		= nextCZA(i);
						xsec1->onCeilZ		= sec1->floorz;
						xsec1->crush		= 1;

						xsec1->busyTimeA 	= xsec1->busyTimeB = sec1vel & 255;
						xsec1->reTriggerA	= xsec1->reTriggerB	= 1;
						xsec1->busyWaveB	= 3;
						
						if (sec1lo == 33)
							xsec1->rxID = 7;
						
						// let's create a little earthquake here...
						avePointSector(i, &avex, &avey);
						if ((k = insertEarthquake(i, avex, avey, sec1->floorz, 150 - xsec1->busyTimeA)))
						{
							xspr1 = GetPXSprite(k); xspr1->rxID = findUnusedChannel();
							xsec1->txID = xspr1->rxID;
							xsec1->triggerOn = 1;
						}
						break;
				}
				sec1->type = kSectorZMotion;
				break;
			case 40:
			{
				//break;
				xsec1 = GetPXSector(i);
				int nFirst = findFirstWayPoint(i); dassert(nFirst >= 0);
				int nPrev = nFirst; int nCur = nFirst, nId = sprite[nFirst].type;
				
				int nFirst2 = nFirst;
				if (sprite[nFirst].sectnum != i)
				{
					wayPointInc(sec1hi);
					k = InsertSprite(i, 0);
					avePointSector(i, &avex, &avey);
					sprite[k].x = avex, sprite[k].y = avey; sprite[k].z = sec1->floorz;
					sprite[k].type = 900;
					sprite[k].hitag = sec1hi;
					nFirst2 = findFirstWayPoint(i); dassert(nFirst2 >= 0);
					nPrev = nFirst2, nId = sprite[nFirst2].type;
					
				}
				
				int nIdNew = findUnusedPath();
				xsec1->data = nIdNew;
				xsec1->rxID = 7;
				xsec1->drag = 1;
				sec1->floorstat |= kSectRelAlign;
				while( 1 )
				{
					spr1 = &sprite[nPrev]; xspr1 = GetPXSprite(nPrev);
					ChangeSpriteStat(spr1->index, kStatPathMarker);
					spr1->type 		= kMarkerPath; spr1->ang		= 0; spr1->clipdist	= 8;
					spr1->z			= sprite[nFirst].z;
					spr1->cstat		|= kSprFloor;
					
					xspr1->data1	= nIdNew;
					xspr1->wave		= 1;
					
					if ((nCur = findNextWayPoint(sec1hi, nId)) >= 0)
					{
						spr2 = &sprite[nCur]; nIdNew = findUnusedPath(); nId = spr2->type; 
						xspr1->data2 = nIdNew;
						//Alert("%d", i);
						//int time = 0;
						int dx = sprite[nCur].x - sprite[nPrev].x;
						int dy = sprite[nCur].y - sprite[nPrev].y;
						int dist = approxDist(dx, dy);
						int time = (ClipLow(dist / sec1->type, 1) *120) / 10;


						xspr1->busyTime	= time;
						
						
						xspr2 = GetPXSprite(spr2->index);
						ChangeSpriteStat(spr2->index, kStatPathMarker);
						spr2->type = kMarkerPath;
						spr2->ang		= 0;
						spr2->clipdist	= 8;
						spr1->z			= sprite[nFirst].z;
						spr1->cstat		|= kSprFloor;
						xspr2->data1 	= nIdNew;
						xspr2->wave		= 1;
						xspr2->busyTime	= time;
						
						
						nPrev = nCur;
					}
					else
					{
						spr2 = &sprite[nFirst]; xspr2 = GetPXSprite(spr2->index);
						xspr1->data2 = xspr2->data1;
						break;
					}
				}
				sec1->type = kSectorPath;
				break;
			}
			case 45:
				sec1->floorz = nextFZB(i);
				k = InsertSprite(i, kStatDude);
				sprite[k].type = kDudeGargoyleStatueFlesh;
				avePointSector(i, &sprite[k].x, &sprite[k].y);
				PutSpriteOnFloor(&sprite[k], 0);
				break;
			case 21:
				xsec1 = GetPXSector(i);
				xsec1->triggerEnter = 1;
				xsec1->dudeLockout = 1;
				k = findUnusedChannel();
				for (j = 0; j < numsectors; j++)
				{
					if (j != i && sector[j].hitag == sector[i].hitag)
					{
						xsec2 = GetPXSector(j);
						xsec2->rxID = k;
					}
				}
				xsec1->txID = k;
				xsec1->triggerOn = 1;
				xsec1->command = 1;
				sec1->type = 0;
				break;
		}
	}
	
	int wal1vel;
	for (i = 0; i < numwalls; i++)
	{
		wal1 = &wall[i];
		switch(wal1->type) {
			case 7:
			case 8:
			case 9:
				wal1->type = 20;
				xwal1 = GetPXWall(i);
				xwal1->command = (wal1->type == 8) ? 0 : 1;
				k = findUnusedChannel();
				for (j = 0; j < numsectors; j++)
				{
					if (sector[j].hitag == wal1->hitag)
					{
						xsec1 = GetPXSector(j);
						if (xsec1->rxID >= 100) // already connected with something else
						{
							Alert("%d", j);
							int x, y;
							avePointSector(j, &x, &y);
							//f = insertSwitch(j, x, y, sector[j].floorz, k, xsec1->rxID, xwal1->command);
						}
						else
						{
							//dassert(xsec1->rxID <= 0);
							xsec1->rxID = k;
						}
					}
				}
				xwal1->txID = k;
				xwal1->triggerOn = xwal1->triggerOff = 1;
				xwal1->triggerPush = 1;
				xwal1->triggerOnce = 1; // !!!
				break;
			
		}
	}
	
	return 0;
	
}
#endif

int dbLoadBuildMap(IOBuffer* pIo) {
	
	int i, len, ver = 0;
	SECTOR4 sector4; WALL4 wall4; SPRITE4 sprite4;
	SECTOR6 sector6; WALL6 wall6; SPRITE6 sprite6;
	pIo->seek(0, SEEK_SET);
	
	pIo->read(&ver, 4);
	len = LENGTH(gSuppBuildMapVersion);
	for (i = 0; i < len && ver != gSuppBuildMapVersion[i]; i++);
	if (i >= len)
		return -2;
	
	gCustomSkyBits = TRUE;
	gSkyCount	= 1;
	pskyoff[0]	= 0;
	dbInit();
	
	pIo->read(&startposx, 		4);								// x-pos
	pIo->read(&startposy, 		4);								// y-pos
	pIo->read(&startposz, 		4);								// z-pos
	pIo->read(&startang, 		2);								// angle
	pIo->read(&startsectnum, 	2);								// start sector

	switch(ver) {
		case 7L:
		case 8L:
		case 9L:
			pIo->read(&numsectors, 	2);							// read numsectors
			pIo->read(sector, sizeof(sectortype)*numsectors);	// fill the array
			pIo->read(&numwalls, 	2);	 						// read numwalls
			pIo->read(wall, sizeof(walltype)*numwalls);			// fill the array
			pIo->read(&numsprites, 	2);							// read numsprites
			for (i =0; i < numsprites; i++)						// insert sprites
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite[i], sizeof(spritetype));
				InsertSpriteSect(i, sprite[i].sectnum);
				InsertSpriteStat(i, sprite[i].statnum);
				sprite[i].index 	= i;
			}
			break;
		case 4L:
			// this is based on Ken's convmaps
			pIo->read(&numsectors, 	2);							// read numsectors
			pIo->read(&numwalls, 	2);	 						// read numwalls
			pIo->read(&numsprites, 	2);							// read numsprites
			for (i = 0; i < numsectors; i++)					// convert each sector
			{
				pIo->read(&sector4, sizeof(SECTOR4));
				sector[i].wallptr 			= sector4.wallptr;
				sector[i].wallnum 			= sector4.wallnum;
				sector[i].ceilingshade 		= sector4.ceilingshade;
				sector[i].ceilingz 			= sector4.ceilingz;
				sector[i].ceilingpicnum 	= sector4.ceilingpicnum;
				sector[i].ceilingslope 		= 0;
				sector[i].floorshade 		= sector4.floorshade;
				sector[i].floorz 			= sector4.floorz;
				sector[i].floorpicnum 		= sector4.floorpicnum;
				sector[i].floorslope 		= 0;
				sector[i].type 				= (short)(sector4.tag & 65535);
				sector[i].hitag 			= (short)(sector4.tag >> 16);

				sector[i].ceilingstat = (short)(sector4.ceilingstat & ~4);
				if ((sector[i].ceilingstat & 4) == 0)
				{
					sector[i].ceilingypanning = 0;
					sector[i].ceilingxpanning = sector4.ceilingxpanning;
				}
				else
				{
					sector[i].ceilingxpanning = 0;
					sector[i].ceilingypanning = sector4.ceilingypanning;
				}

				sector[i].floorstat = (short)(sector4.floorstat & ~4);
				if ((sector[i].floorstat&4) == 0)
				{
					sector[i].floorypanning = 0;
					sector[i].floorxpanning = sector4.floorxpanning;
				}
				else
				{
					sector[i].floorxpanning = 0;
					sector[i].floorypanning = sector4.floorypanning;
				}
			}
			for (i = 0; i < numwalls; i++)							// convert each wall
			{
				pIo->read(&wall4, sizeof(WALL4));
				wall[i].x 				= wall4.x;
				wall[i].y 				= wall4.y;
				wall[i].point2 			= wall4.point2;
				wall[i].nextsector 		= wall4.nextsector1;
				wall[i].nextwall 		= wall4.nextwall1;
				wall[i].picnum 			= wall4.picnum;
				wall[i].overpicnum 		= wall4.overpicnum;
				wall[i].shade 			= wall4.shade;
				wall[i].pal 			= sector[sectorofwall((short)i)].floorpal;
				wall[i].cstat 			= wall4.cstat;
				wall[i].xrepeat 		= wall4.xrepeat;
				wall[i].yrepeat 		= wall4.yrepeat;
				wall[i].xpanning 		= wall4.xpanning;
				wall[i].ypanning 		= wall4.ypanning;
				wall[i].type 			= (short)(wall4.tag & 65535);
				wall[i].hitag 			= (short)(wall4.tag >> 16);
			}
			for (i = 0; i < numsprites; i++)						// convert each sprite
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite4, sizeof(SPRITE4));
				InsertSpriteSect(i, sprite4.sectnum);
				InsertSpriteStat(i, sprite4.statnum);
				sprite[i].index 		= i;
				sprite[i].x 			= sprite4.x;
				sprite[i].y 			= sprite4.y;
				sprite[i].z 			= sprite4.z;
				sprite[i].cstat 		= sprite4.cstat;
				sprite[i].shade 		= sprite4.shade;
				sprite[i].xrepeat 		= sprite4.xrepeat;
				sprite[i].yrepeat		= sprite4.yrepeat;
				sprite[i].pal			= 0;
				sprite[i].xoffset		= 0;
				sprite[i].yoffset		= 0;
				sprite[i].clipdist		= 32;
				sprite[i].picnum 		= sprite4.picnum;
				sprite[i].ang 			= sprite4.ang;
				sprite[i].yvel	 		= sprite4.yvel;
				sprite[i].zvel	 		= sprite4.zvel;
				sprite[i].owner 		= sprite4.owner;
				sprite[i].sectnum 		= sprite4.sectnum;
				sprite[i].statnum 		= sprite4.statnum;
				sprite[i].type 			= (short)(sprite4.tag & 65535);
				sprite[i].flags 		= (short)(sprite4.tag >> 16);
							
				if ((sprite[i].cstat & kSprRelMask) == 48)
					sprite[i].cstat &= ~48;
			}
			break;
		case 6L:
			// this is based on eduke32 v6->v7 conversion
			pIo->read(&numsectors, 2);								// read numsectors
			for (i = 0; i < numsectors; i++)						// convert each one
			{
				pIo->read(&sector6, sizeof(SECTOR6));
				sector[i].wallptr 			= sector6.wallptr;
				sector[i].wallnum 			= sector6.wallnum;
				sector[i].ceilingstat		= sector6.ceilingstat;
				sector[i].ceilingz 			= sector6.ceilingz;
				sector[i].ceilingpicnum 	= sector6.ceilingpicnum;
				sector[i].ceilingshade 		= sector6.ceilingshade;
				sector[i].ceilingpal 		= sector6.ceilingpal;
				sector[i].ceilingxpanning 	= sector6.ceilingxpanning;
				sector[i].ceilingypanning 	= sector6.ceilingypanning;
				sector[i].floorstat			= sector6.floorstat;
				sector[i].floorz 			= sector6.floorz;
				sector[i].floorpicnum 		= sector6.floorpicnum;
				sector[i].floorshade 		= sector6.floorshade;
				sector[i].floorpal 			= sector6.floorpal;
				sector[i].floorxpanning 	= sector6.floorxpanning;
				sector[i].floorypanning 	= sector6.floorypanning;
				sector[i].visibility 		= sector6.visibility;
				sector[i].type 				= sector6.lotag;
				sector[i].hitag 			= sector6.hitag;
				//sector[i].extra				= sector6.extra;

				sector[i].ceilingslope = (short)max(min(((int)sector6.ceilingheinum) << 5, 32767), -32768);
				if ((sector6.ceilingstat & 2) == 0)
					sector[i].ceilingslope = 0;

				sector[i].floorslope = (short)max(min(((int)sector6.floorheinum) << 5, 32767), -32768);
				if ((sector6.floorstat & 2) == 0)
					sector[i].floorslope = 0;
			}

			pIo->read(&numwalls, 2);	 							// read numwalls
			for (i = 0; i < numwalls; i++)							// convert each one
			{
				pIo->read(&wall6, sizeof(WALL6));
				wall[i].x 			= wall6.x;
				wall[i].y 			= wall6.y;
				wall[i].point2 		= wall6.point2;
				wall[i].nextwall 	= wall6.nextwall;
				wall[i].nextsector 	= wall6.nextsector;
				wall[i].cstat 		= wall6.cstat;
				wall[i].picnum 		= wall6.picnum;
				wall[i].overpicnum 	= wall6.overpicnum;
				wall[i].shade 		= wall6.shade;
				wall[i].pal 		= wall6.pal;
				wall[i].xrepeat 	= wall6.xrepeat;
				wall[i].yrepeat 	= wall6.yrepeat;
				wall[i].xpanning 	= wall6.xpanning;
				wall[i].ypanning 	= wall6.ypanning;
				wall[i].type 		= wall6.lotag;
				wall[i].hitag 		= wall6.hitag;
				//wall[i].extra		= wall6.extra;
			}

			pIo->read(&numsprites, sizeof(short));						// read numsprites
			for (i = 0; i < numsprites; i++)							// convert each one
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite6, sizeof(SPRITE6));
				InsertSpriteSect(i, sprite6.sectnum);
				InsertSpriteStat(i, sprite6.statnum);
				sprite[i].index 	= i;
				sprite[i].x 		= sprite6.x;
				sprite[i].y 		= sprite6.y;
				sprite[i].z 		= sprite6.z;
				sprite[i].cstat 	= sprite6.cstat;
				sprite[i].picnum 	= sprite6.picnum;
				sprite[i].shade 	= sprite6.shade;
				sprite[i].pal 		= sprite6.pal;
				sprite[i].clipdist 	= sprite6.clipdist;
				sprite[i].xrepeat 	= sprite6.xrepeat;
				sprite[i].yrepeat 	= sprite6.yrepeat;
				sprite[i].xoffset 	= sprite6.xoffset;
				sprite[i].yoffset 	= sprite6.yoffset;
				sprite[i].owner		= sprite6.owner;
				sprite[i].sectnum 	= sprite6.sectnum;
				sprite[i].statnum 	= sprite6.statnum;
				sprite[i].ang 		= sprite6.ang;
				sprite[i].yvel 		= sprite6.yvel;
				sprite[i].zvel 		= sprite6.zvel;
				sprite[i].type 		= sprite6.lotag;
				sprite[i].flags 	= sprite6.hitag;
				//sprite[i].extra		= sprite6.extra;
							
				if ((sprite[i].cstat & kSprRelMask) == 48)
					sprite[i].cstat &= ~48;
			}
			break;
	}

	updatenumsprites();
	eraseExtra();
	
	for (i = 0; i < numsprites; i++)
	{
		if (!gMapImport.eraseSprites)
		{
			//sprite[i].owner		= -1;
			//sprite[i].detail	= 0;
			//sprite[i].type = (short)ClipRange(sprite[i].type, 0, gMapImport.eraseTypes ? 0 : 1023);

			if (gMapImport.sprite2pic) sprite[i].picnum = gMapImport.sprite2pic;
			if (gMapImport.erasePals)  sprite[i].pal = 0;
		}
		else
		{
			DeleteSprite(i);
		}
	}
	
	for (i = 0; i < numwalls; i++)
	{
		if (gMapImport.erasePals) wall[i].pal = 0;
		if (gMapImport.wall2pic) wall[i].picnum = gMapImport.wall2pic;
		if (gMapImport.wallOver2pic) wall[i].overpicnum = gMapImport.wallOver2pic;
		//wall[i].type = ClipRange(wall[i].type, 0, gMapImport.eraseTypes ? 0 : 1023);
	}

	for (i = 0; i < numsectors; i++)
	{
		sector[i].alignto = 0;
		//sector[i].type = ClipRange(sector[i].type, 0, gMapImport.eraseTypes ? 0 : 1023);
		if (sector[i].floorstat & kSectParallax)
		{
			if (gMapImport.parallax2pic)
				sector[i].floorpicnum = gMapImport.parallax2pic;

			SetupSky(sector[i].floorpicnum);
		}
		else if (gMapImport.floor2pic)
		{
			sector[i].floorpicnum = gMapImport.floor2pic;
		}

		if (sector[i].ceilingstat & kSectParallax)
		{
			if (gMapImport.parallax2pic)
				sector[i].ceilingpicnum = gMapImport.parallax2pic;

			SetupSky(sector[i].ceilingpicnum);

		} else if (gMapImport.ceil2pic)
		{
			sector[i].ceilingpicnum = gMapImport.ceil2pic;
		}

		if (gMapImport.erasePals) sector[i].floorpal = sector[i].ceilingpal = 0;
		
	}
	
	//exhum2xsystem();
	//AutoAdjustSprites();
		
	posx = startposx;
	posy = startposy;
	posz = startposz;
	ang	 = startang;

	updatesector(posx, posy, &cursectnum);
	return ver;
}

int dbLoadBuildMap(const char *mapname) {

	int hFile, len, retn; char* pData;
	if ((hFile = open(mapname, O_RDONLY|O_BINARY, S_IWRITE)) < 0) return -1;
	else if ((len = filelength(hFile)) <= 0)
	{
		close(hFile);
		return -1;
	}

	pData = (char*)Resource::Alloc(len); read(hFile, pData, len); close(hFile);
	IOBuffer iobuf= IOBuffer(len, pData);
	
	retn = dbLoadBuildMap(&iobuf);
	Resource::Free(pData);
	return retn;
}

int dbLoadMap(char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector) {

	int i, nSize, hFile; bool ERROR = true;
	char *pData, *dsk, *dir, *fname, *ext;
	char mapname[BMAX_PATH], tmp[BMAX_PATH];
	RESHANDLE hMap = NULL;
	MAPSIGNATURE magic;
	MAPHEADER header;

	int16_t tpskyoff[256];

	memset(show2dsector, 0, sizeof(show2dsector));
    memset(show2dwall, 0, sizeof(show2dwall));
    memset(show2dsprite, 0, sizeof(show2dsprite));

	gMapRev = 0;
	gModernMap = false;
	numxsprites = numxwalls = numxsectors = 0;

	pathSplit2(pPath, tmp, &dsk, &dir, &fname, &ext);
	sprintf(mapname, pPath);
	ChangeExtension(mapname, getExt(kMap));

	// check if file exists on the disk or inside RFF?
	if ((i = fileExists(mapname, &hMap)) == 0) { ThrowError("%s is not exists!", mapname); }
	else if ((i & 0x01) && (hFile = open(mapname, O_RDONLY | O_BINARY)) >= 0) // file on the disk is the priority
	{
		if ((nSize = filelength(hFile)) > 0)
		{
			pData = (char*)Resource::Alloc(nSize);
			ERROR  = (read(hFile, (char*)pData, nSize) < nSize);
		}

		close(hFile);
	}
	else if (hMap && (nSize = gSysRes.Size(hMap)) > 0) // load map from the RFF then
	{
		pData = (char*)Resource::Alloc(nSize);
		memcpy(pData, gSysRes.Load(hMap), nSize);
		ERROR = false;
	}

	if (ERROR)
		ThrowError("Error loading map file %s", pPath);

    mapversion = 7;
	IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    IOBuffer1.read(&magic, 6);

#if B_BIG_ENDIAN == 1
    magic.version = B_LITTLE16(magic.version);
#endif

	if (memcmp(magic.signature, "BLM\x1a", 4) != 0)
	{
		// let's try to load it as BUILD map...
		switch (dbLoadBuildMap(&IOBuffer1)) {
			case -2:
				ThrowError("Unsupported version or not a BUILD board file!");
				break;
			default:
				break;
		}

		Resource::Free(pData);
		return 0; // success!
	}

    gByte1A76C8 = false;
	if ((magic.version & 0xFF00) != (0x0603 & 0xFF00))
	{
		if ((magic.version & 0x00FF) == 0x001) gModernMap = true; // the map uses modern features
		if ((magic.version & 0xFF00) == (0x0700 & 0xFF00)) gByte1A76C8 = true;
		else ThrowError("Map file is wrong version");
	}

    IOBuffer1.read(&header, 37);
    if (header.at16 != 0 && header.at16 != 0x7474614d && header.at16 != 0x4d617474)
	{
        dbCrypt((char*)&header, sizeof(header), 0x7474614d);
        gByte1A76C7 = true;
    }

#if B_BIG_ENDIAN == 1
    header.at0 = B_LITTLE32(header.at0);
    header.at4 = B_LITTLE32(header.at4);
    header.at8 = B_LITTLE32(header.at8);
    header.atc = B_LITTLE16(header.atc);
    header.ate = B_LITTLE16(header.ate);
    header.at10 = B_LITTLE16(header.at10);
    header.at12 = B_LITTLE32(header.at12);
    header.at16 = B_LITTLE32(header.at16);
    header.at1b = B_LITTLE32(header.at1b);
    header.at1f = B_LITTLE16(header.at1f);
    header.at21 = B_LITTLE16(header.at21);
    header.at23 = B_LITTLE16(header.at23);
#endif

    *pX = header.at0;
    *pY = header.at4;
    *pZ = header.at8;
    *pAngle = header.atc;
    *pSector = header.ate;

	pskybits = header.at10;
    visibility = header.at12;
    gSongId = header.at16;

    if (gByte1A76C8)
    {
        if (header.at16 == 0x7474614d || header.at16 == 0x4d617474) gByte1A76C6 = true;
        else if (!header.at16) gByte1A76C6 = 0;
        else
        {
			ThrowError("Corrupted Map file");
        }
    }
    else if (header.at16)
    {
		ThrowError("Corrupted Map file");
    }

    parallaxtype 	= header.at1a;
    gMapRev 		= header.at1b;
    numsectors 		= header.at1f;
    numwalls 		= header.at21;
    dbInit();

    if (gByte1A76C8)
    {
        IOBuffer1.read(&mapHead, 128);
        dbCrypt((char*)&mapHead, 128, numwalls);
#if B_BIG_ENDIAN == 1
        mapHead.at40 = B_LITTLE32(mapHead.at40);
        mapHead.at44 = B_LITTLE32(mapHead.at44);
        mapHead.at48 = B_LITTLE32(mapHead.at48);
#endif
    }
    else
    {
        memset(&mapHead, 0, 128);
    }
	
	/////////////////
	if (gCmtPrefs.enabled)
	{
		gCommentMgr.DeleteAll();
		char cmtfile[_MAX_PATH];
		sprintf(cmtfile, pPath);
		ChangeExtension(cmtfile, kCommentExt);
		if (fileExists(cmtfile))
		{
			IniFile* pComment = new IniFile(cmtfile);
			gCommentMgr.LoadFromIni(pComment);
			delete(pComment);
		}
	}
	//////
	
	gSkyCount = 1<<pskybits;
    IOBuffer1.read(tpskyoff, gSkyCount*sizeof(tpskyoff[0]));
    if (gByte1A76C8)
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*2);

	for (i = 0; i < ClipHigh(gSkyCount, MAXPSKYTILES); i++)
        pskyoff[i] = B_LITTLE16(tpskyoff[i]);

    for (i = 0; i < numsectors; i++)
    {
        sectortype *pSector = &sector[i];
        IOBuffer1.read(pSector, sizeof(sectortype));
		
        if (gByte1A76C8)
        {
            dbCrypt((char*)pSector, sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
#if B_BIG_ENDIAN == 1
        pSector->wallptr = B_LITTLE16(pSector->wallptr);
        pSector->wallnum = B_LITTLE16(pSector->wallnum);
        pSector->ceilingz = B_LITTLE32(pSector->ceilingz);
        pSector->floorz = B_LITTLE32(pSector->floorz);
        pSector->ceilingstat = B_LITTLE16(pSector->ceilingstat);
        pSector->floorstat = B_LITTLE16(pSector->floorstat);
        pSector->ceilingpicnum = B_LITTLE16(pSector->ceilingpicnum);
        pSector->ceilingheinum = B_LITTLE16(pSector->ceilingheinum);
        pSector->floorpicnum = B_LITTLE16(pSector->floorpicnum);
        pSector->floorheinum = B_LITTLE16(pSector->floorheinum);
        pSector->type = B_LITTLE16(pSector->type);
        pSector->hitag = B_LITTLE16(pSector->hitag);
        pSector->extra = B_LITTLE16(pSector->extra);
#endif
        if (sector[i].extra > 0)
        {
			XSECTOR *pXSector;
			char pBuffer[nXSectorSize];
			int oExtra, nXSector, nCount;

			oExtra   = pSector->extra;
			nXSector = dbInsertXSector(i);
            pXSector = &xsector[nXSector];
            nCount   = (gByte1A76C8) ? mapHead.at48 : nXSectorSize;

			dassert(nCount <= nXSectorSize);
			memset(pXSector, 0, sizeof(XSECTOR));

			// rebind from old xsector to newly created
			if (gCmtPrefs.enabled)
				gCommentMgr.RebindMatching(OBJ_SECTOR, oExtra, OBJ_SECTOR, nXSector, TRUE);
			
			IOBuffer1.read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXSector->reference = bitReader.readSigned(14);
            pXSector->state = bitReader.readUnsigned(1);
            pXSector->busy = bitReader.readUnsigned(17);
            pXSector->data = bitReader.readUnsigned(16);
            pXSector->txID = bitReader.readUnsigned(10);
            pXSector->busyWaveA = bitReader.readUnsigned(3);
            pXSector->busyWaveB = bitReader.readUnsigned(3);
            pXSector->rxID = bitReader.readUnsigned(10);
            pXSector->command = bitReader.readUnsigned(8);
            pXSector->triggerOn = bitReader.readUnsigned(1);
            pXSector->triggerOff = bitReader.readUnsigned(1);
            pXSector->busyTimeA = bitReader.readUnsigned(12);
            pXSector->waitTimeA = bitReader.readUnsigned(12);
            pXSector->restState = bitReader.readUnsigned(1);
            pXSector->interruptable = bitReader.readUnsigned(1);
            pXSector->amplitude = bitReader.readSigned(8);
            pXSector->shadeFreq = bitReader.readUnsigned(8);
            pXSector->reTriggerA = bitReader.readUnsigned(1);
            pXSector->reTriggerB = bitReader.readUnsigned(1);
            pXSector->shadePhase = bitReader.readUnsigned(8);
            pXSector->shadeWave = bitReader.readUnsigned(4);
            pXSector->shadeAlways = bitReader.readUnsigned(1);
            pXSector->shadeFloor = bitReader.readUnsigned(1);
            pXSector->shadeCeiling = bitReader.readUnsigned(1);
            pXSector->shadeWalls = bitReader.readUnsigned(1);
            pXSector->shade = bitReader.readSigned(8);
            pXSector->panAlways = bitReader.readUnsigned(1);
            pXSector->panFloor = bitReader.readUnsigned(1);
            pXSector->panCeiling = bitReader.readUnsigned(1);
            pXSector->drag = bitReader.readUnsigned(1);
            pXSector->underwater = bitReader.readUnsigned(1);
            pXSector->Depth = bitReader.readUnsigned(3);
            pXSector->panVel = bitReader.readUnsigned(8);
            pXSector->panAngle = bitReader.readUnsigned(11);
            pXSector->unused1 = bitReader.readUnsigned(1);
            pXSector->decoupled = bitReader.readUnsigned(1);
            pXSector->triggerOnce = bitReader.readUnsigned(1);
            pXSector->isTriggered = bitReader.readUnsigned(1);
            pXSector->key = bitReader.readUnsigned(3);
            pXSector->triggerPush = bitReader.readUnsigned(1);
            pXSector->triggerVector = bitReader.readUnsigned(1);
            pXSector->triggerReserved = bitReader.readUnsigned(1);
            pXSector->triggerEnter = bitReader.readUnsigned(1);
            pXSector->triggerExit = bitReader.readUnsigned(1);
            pXSector->triggerWallPush = bitReader.readUnsigned(1);
            pXSector->coloredLights = bitReader.readUnsigned(1);
            pXSector->unused2 = bitReader.readUnsigned(1);
            pXSector->busyTimeB = bitReader.readUnsigned(12);
            pXSector->waitTimeB = bitReader.readUnsigned(12);
            pXSector->stopOn = bitReader.readUnsigned(1);
            pXSector->stopOff = bitReader.readUnsigned(1);
            pXSector->ceilpal2 = bitReader.readUnsigned(4);
            pXSector->offCeilZ = bitReader.readSigned(32);
            pXSector->onCeilZ = bitReader.readSigned(32);
            pXSector->offFloorZ = bitReader.readSigned(32);
            pXSector->onFloorZ = bitReader.readSigned(32);
            pXSector->marker0 = bitReader.readUnsigned(16);
            pXSector->marker1 = bitReader.readUnsigned(16);
            pXSector->crush = bitReader.readUnsigned(1);
            pXSector->ceilXPanFrac = bitReader.readUnsigned(8);
            pXSector->ceilYPanFrac = bitReader.readUnsigned(8);
            pXSector->floorXPanFrac = bitReader.readUnsigned(8);
            pXSector->damageType = bitReader.readUnsigned(3);
            pXSector->floorpal2 = bitReader.readUnsigned(4);
            pXSector->floorYPanFrac = bitReader.readUnsigned(8);
            pXSector->locked = bitReader.readUnsigned(1);
            pXSector->windVel = bitReader.readUnsigned(10);
            pXSector->windAng = bitReader.readUnsigned(11);
            pXSector->windAlways = bitReader.readUnsigned(1);
            pXSector->dudeLockout = bitReader.readUnsigned(1);
            pXSector->bobTheta = bitReader.readUnsigned(11);
            pXSector->bobZRange = bitReader.readUnsigned(5);
            pXSector->bobSpeed = bitReader.readSigned(12);
            pXSector->bobAlways = bitReader.readUnsigned(1);
            pXSector->bobFloor = bitReader.readUnsigned(1);
            pXSector->bobCeiling = bitReader.readUnsigned(1);
            pXSector->bobRotate = bitReader.readUnsigned(1);
            xsector[sector[i].extra].reference = i;
            xsector[sector[i].extra].busy = xsector[sector[i].extra].state<<16;

        }
    }
    for (i = 0; i < numwalls; i++)
    {
        walltype *pWall = &wall[i];
        IOBuffer1.read(pWall, sizeof(walltype));
        if (gByte1A76C8)
            dbCrypt((char*)pWall, sizeof(walltype), (gMapRev*sizeof(sectortype)) | 0x7474614d);
		
#if B_BIG_ENDIAN == 1
        pWall->x = B_LITTLE32(pWall->x);
        pWall->y = B_LITTLE32(pWall->y);
        pWall->point2 = B_LITTLE16(pWall->point2);
        pWall->nextwall = B_LITTLE16(pWall->nextwall);
        pWall->nextsector = B_LITTLE16(pWall->nextsector);
        pWall->cstat = B_LITTLE16(pWall->cstat);
        pWall->picnum = B_LITTLE16(pWall->picnum);
        pWall->overpicnum = B_LITTLE16(pWall->overpicnum);
        pWall->type = B_LITTLE16(pWall->type);
        pWall->hitag = B_LITTLE16(pWall->hitag);
        pWall->extra = B_LITTLE16(pWall->extra);
#endif
        if (wall[i].extra > 0)
        {
			XWALL *pXWall;
			char pBuffer[nXWallSize];
			int oExtra, nXWall, nCount;

			oExtra = wall[i].extra;
			nXWall = dbInsertXWall(i);
            pXWall = &xwall[nXWall];
            nCount = (gByte1A76C8) ? mapHead.at44 : nXWallSize;
            
			dassert(nCount <= nXWallSize);
			memset(pXWall, 0, sizeof(XWALL));
			

			// rebind from old xwall to newly created
			if (gCmtPrefs.enabled)
				gCommentMgr.RebindMatching(OBJ_WALL, oExtra, OBJ_WALL, nXWall, TRUE);

	
            IOBuffer1.read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXWall->reference = bitReader.readSigned(14);
            pXWall->state = bitReader.readUnsigned(1);
            pXWall->busy = bitReader.readUnsigned(17);
            pXWall->data = bitReader.readSigned(16);
            pXWall->txID = bitReader.readUnsigned(10);
            pXWall->unused1 = bitReader.readUnsigned(6);
            pXWall->rxID = bitReader.readUnsigned(10);
            pXWall->command = bitReader.readUnsigned(8);
            pXWall->triggerOn = bitReader.readUnsigned(1);
            pXWall->triggerOff = bitReader.readUnsigned(1);
            pXWall->busyTime = bitReader.readUnsigned(12);
            pXWall->waitTime = bitReader.readUnsigned(12);
            pXWall->restState = bitReader.readUnsigned(1);
            pXWall->interruptable = bitReader.readUnsigned(1);
            pXWall->panAlways = bitReader.readUnsigned(1);
            pXWall->panXVel = bitReader.readSigned(8);
            pXWall->panYVel = bitReader.readSigned(8);
            pXWall->decoupled = bitReader.readUnsigned(1);
            pXWall->triggerOnce = bitReader.readUnsigned(1);
            pXWall->isTriggered = bitReader.readUnsigned(1);
            pXWall->key = bitReader.readUnsigned(3);
            pXWall->triggerPush = bitReader.readUnsigned(1);
            pXWall->triggerVector = bitReader.readUnsigned(1);
            pXWall->triggerTouch = bitReader.readUnsigned(1);
            pXWall->unused2 = bitReader.readUnsigned(2);
            pXWall->xpanFrac = bitReader.readUnsigned(8);
            pXWall->ypanFrac = bitReader.readUnsigned(8);
            pXWall->locked = bitReader.readUnsigned(1);
            pXWall->dudeLockout = bitReader.readUnsigned(1);
            pXWall->unused3 = bitReader.readUnsigned(4);
            pXWall->unused4 = bitReader.readUnsigned(32);
            xwall[wall[i].extra].reference = i;
            xwall[wall[i].extra].busy = xwall[wall[i].extra].state << 16;
			


        }
    }

    initspritelists();

	for (i = 0; i < header.at23; i++)
    {
        RemoveSpriteStat(i);
        spritetype *pSprite = &sprite[i];
        IOBuffer1.read(pSprite, sizeof(spritetype));
        if (gByte1A76C8)
        {
            dbCrypt((char*)pSprite, sizeof(spritetype), (gMapRev*sizeof(spritetype)) | 0x7474614d);
        }
#if B_BIG_ENDIAN == 1
        pSprite->x = B_LITTLE32(pSprite->x);
        pSprite->y = B_LITTLE32(pSprite->y);
        pSprite->z = B_LITTLE32(pSprite->z);
        pSprite->cstat = B_LITTLE16(pSprite->cstat);
        pSprite->picnum = B_LITTLE16(pSprite->picnum);
        pSprite->sectnum = B_LITTLE16(pSprite->sectnum);
        pSprite->statnum = B_LITTLE16(pSprite->statnum);
        pSprite->ang = B_LITTLE16(pSprite->ang);
        pSprite->owner = B_LITTLE16(pSprite->owner);
        pSprite->index = B_LITTLE16(pSprite->index);
        pSprite->yvel = B_LITTLE16(pSprite->yvel);
        pSprite->inittype = B_LITTLE16(pSprite->inittype);
        pSprite->type = B_LITTLE16(pSprite->type);
        pSprite->flags = B_LITTLE16(pSprite->hitag);
        pSprite->extra = B_LITTLE16(pSprite->extra);
#endif
        InsertSpriteSect(i, sprite[i].sectnum);
        InsertSpriteStat(i, sprite[i].statnum);
        // Numsprites++;
		
		// rebind from old index to newly created
		if (gCmtPrefs.enabled)
			gCommentMgr.RebindMatching(OBJ_SPRITE, sprite[i].index, OBJ_SPRITE, i, TRUE);
        
		sprite[i].index = i;
        // qsprite_filler[i] = pSprite->blend;
        // pSprite->blend = 0;
        if (sprite[i].extra > 0)
        {
            char pBuffer[nXSpriteSize];
            int nXSprite = dbInsertXSprite(i);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            memset(pXSprite, 0, sizeof(XSPRITE));
            int nCount;
            if (!gByte1A76C8)
            {
                nCount = nXSpriteSize;
            }
            else
            {
                nCount = mapHead.at40;
            }
            dassert(nCount <= nXSpriteSize);
            IOBuffer1.read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXSprite->reference = bitReader.readSigned(14);
            pXSprite->state = bitReader.readUnsigned(1);
            pXSprite->busy = bitReader.readUnsigned(17);
            pXSprite->txID = bitReader.readUnsigned(10);
            pXSprite->rxID = bitReader.readUnsigned(10);
            pXSprite->command = bitReader.readUnsigned(8);
            pXSprite->triggerOn = bitReader.readUnsigned(1);
            pXSprite->triggerOff = bitReader.readUnsigned(1);
            pXSprite->wave = bitReader.readUnsigned(2);
            pXSprite->busyTime = bitReader.readUnsigned(12);
            pXSprite->waitTime = bitReader.readUnsigned(12);
            pXSprite->restState = bitReader.readUnsigned(1);
            pXSprite->interruptable = bitReader.readUnsigned(1);
            pXSprite->unused1 = bitReader.readUnsigned(2);
            pXSprite->respawnPending = bitReader.readUnsigned(2);
            pXSprite->unused2 = bitReader.readUnsigned(1);
            pXSprite->lT = bitReader.readUnsigned(1);
            pXSprite->dropItem = bitReader.readUnsigned(8);
            pXSprite->decoupled = bitReader.readUnsigned(1);
            pXSprite->triggerOnce = bitReader.readUnsigned(1);
            pXSprite->isTriggered = bitReader.readUnsigned(1);
            pXSprite->key = bitReader.readUnsigned(3);
            pXSprite->triggerPush = bitReader.readUnsigned(1);
            pXSprite->triggerVector = bitReader.readUnsigned(1);
            pXSprite->triggerImpact = bitReader.readUnsigned(1);
            pXSprite->triggerPickup = bitReader.readUnsigned(1);
            pXSprite->triggerTouch = bitReader.readUnsigned(1);
            pXSprite->triggerSight = bitReader.readUnsigned(1);
            pXSprite->triggerProximity = bitReader.readUnsigned(1);
            pXSprite->unused3 = bitReader.readUnsigned(2);
            pXSprite->lSkill = bitReader.readUnsigned(5);
            pXSprite->lS = bitReader.readUnsigned(1);
            pXSprite->lB = bitReader.readUnsigned(1);
            pXSprite->lC = bitReader.readUnsigned(1);
            pXSprite->dudeLockout = bitReader.readUnsigned(1);
            pXSprite->data1 = bitReader.readSigned(16);
            pXSprite->data2 = bitReader.readSigned(16);
            pXSprite->data3 = bitReader.readSigned(16);
            pXSprite->goalAng = bitReader.readUnsigned(11);
            pXSprite->dodgeDir = bitReader.readSigned(2);
            pXSprite->locked = bitReader.readUnsigned(1);
            pXSprite->medium = bitReader.readUnsigned(2);
            pXSprite->respawn = bitReader.readUnsigned(2);
            pXSprite->data4 = bitReader.readUnsigned(16);
            pXSprite->unused4 = bitReader.readUnsigned(6);
            pXSprite->lockMsg = bitReader.readUnsigned(8);
            pXSprite->health = bitReader.readUnsigned(12);
            pXSprite->dudeDeaf = bitReader.readUnsigned(1);
            pXSprite->dudeAmbush = bitReader.readUnsigned(1);
            pXSprite->dudeGuard = bitReader.readUnsigned(1);
            pXSprite->dudeFlag4 = bitReader.readUnsigned(1);
            pXSprite->target = bitReader.readSigned(16);
            pXSprite->targetX = bitReader.readSigned(32);
            pXSprite->targetY = bitReader.readSigned(32);
            pXSprite->targetZ = bitReader.readSigned(32);
            pXSprite->burnTime = bitReader.readUnsigned(16);
            pXSprite->burnSource = bitReader.readSigned(16);
            pXSprite->height = bitReader.readUnsigned(16);
            pXSprite->stateTimer = bitReader.readUnsigned(16);
            pXSprite->aiState = NULL;
            bitReader.skipBits(32);
            xsprite[sprite[i].extra].reference = i;
            xsprite[sprite[i].extra].busy = xsprite[sprite[i].extra].state << 16;
            if (!gByte1A76C8)
                xsprite[sprite[i].extra].lT |= xsprite[sprite[i].extra].lB;

            if (!gModernMap && pXSprite->rxID == 60 && pXSprite->txID == 60 && pXSprite->command == 100)
				gModernMap = true;
        }

		// remove spin sprite flag for older map versions
		if ((magic.version < 0x0700) && ((sprite[i].cstat & 0x30) == 0x30))
            sprite[i].cstat &= ~0x30;

		#if 0
		if ((sprite[i].cstat & 0x30) == 0x30)
            sprite[i].cstat &= ~0x30;
		#endif

    }

	

	unsigned int nCRC;
    IOBuffer1.read(&nCRC, 4);
	#if B_BIG_ENDIAN == 1
		nCRC = B_LITTLE32(nCRC);
	#endif
	
	if (gCmtPrefs.enabled)
	{
		if (!gCmtPrefs.compareCRC || gCommentMgr.CompareCRC(nCRC))
			gCommentMgr.SetCRC(nCRC);
		else
			gCommentMgr.ResetAllTails();
	}
	
	// who cares?
	// commenting this allows to load alpha maps
/* 	if (crc32once((unsigned char*)pData, nSize-4) != nCRC)
    {
        buildprintf("Map File does not match CRC");
        gSysRes.Unlock(pNode);
        return -1;
    } */

	Resource::Free(pData);
    PropagateMarkerReferences();
    return 0;
}

int dbSaveMap(char *pPath, int nX, int nY, int nZ, short nAngle, short nSector)
{
    char sMapExt[BMAX_PATH];
    char sBakExt[BMAX_PATH];
    int16_t tpskyoff[256];
    int nSpriteNum;
    // psky_t *pSky = tileSetupSky(0);
    gSkyCount = 1<<pskybits;
    gMapRev++;
    nSpriteNum = 0;
    strcpy(sMapExt, pPath);
	strcpy(sBakExt, pPath);
    ChangeExtension(sMapExt, getExt(kMap));
	ChangeExtension(sBakExt, ".bak");
    int nSize = sizeof(MAPSIGNATURE)+sizeof(MAPHEADER);
    if (gByte1A76C8)
    {
        nSize += sizeof(MAPHEADER2);
    }
    for (int i = 0; i < gSkyCount; i++)
        tpskyoff[i] = pskyoff[i];
    nSize += gSkyCount*sizeof(tpskyoff[0]);
    nSize += sizeof(sectortype)*numsectors;
    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].extra > 0)
        {
            nSize += nXSectorSize;
        }
    }
    nSize += sizeof(walltype)*numwalls;
    for (int i = 0; i < numwalls; i++)
    {
        if (wall[i].extra > 0)
        {
            nSize += nXWallSize;
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            nSpriteNum++;
            if (sprite[i].extra > 0)
            {
                nSize += nXSpriteSize;
            }
        }
    }
    nSize += sizeof(spritetype)*nSpriteNum;
    nSize += 4;
    char *pData = (char*)Bmalloc(nSize);
    IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    MAPSIGNATURE magic;
    memcpy(&magic, "BLM\x1a", 4);
    if (gByte1A76C8)
    {
		magic.version = (gCompat.modernMap) ? 0x701 : 0x700;
		gModernMap    = (magic.version == 0x701);
        gByte1A76C7 = 1;
    }
    else
    {
        magic.version = 0x603;
        gByte1A76C7 = 0;
    }
    IOBuffer1.write(&magic, sizeof(magic));
    MAPHEADER mapheader;
    mapheader.at0 = B_LITTLE32(nX);
    mapheader.at4 = B_LITTLE32(nY);
    mapheader.at8 = B_LITTLE32(nZ);
    mapheader.atc = B_LITTLE16(nAngle);
    mapheader.ate = B_LITTLE16(nSector);
    mapheader.at10 = B_LITTLE16(pskybits);
    mapheader.at12 = B_LITTLE32(visibility);
    if (gByte1A76C6)
    {
        gSongId = 0x7474614d;
    }
    else
    {
        gSongId = 0;
    }
    mapheader.at16 = B_LITTLE32(gSongId);
    mapheader.at1a = parallaxtype;
    mapheader.at1b = gMapRev;
    mapheader.at1f = B_LITTLE16(numsectors);
    mapheader.at21 = B_LITTLE16(numwalls);
    mapheader.at23 = B_LITTLE16(nSpriteNum);
    if (gByte1A76C7)
    {
        dbCrypt((char*)&mapheader, sizeof(MAPHEADER), 'ttaM');
    }
    IOBuffer1.write(&mapheader, sizeof(MAPHEADER));
    if (gByte1A76C8)
    {
        Bstrcpy(mapHead.at0, "");
        mapHead.at48 = nXSectorSize;
        mapHead.at44 = nXWallSize;
        mapHead.at40 = nXSpriteSize;
        dbCrypt((char*)&mapHead, sizeof(MAPHEADER2), numwalls);
        IOBuffer1.write(&mapHead, sizeof(MAPHEADER2));
        dbCrypt((char*)&mapHead, sizeof(MAPHEADER2), numwalls);
    }
    if (gByte1A76C8)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*sizeof(tpskyoff[0]));
    }
    IOBuffer1.write(tpskyoff, gSkyCount*sizeof(tpskyoff[0]));
    if (gByte1A76C8)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*sizeof(tpskyoff[0]));
    }
    for (int i = 0; i < numsectors; i++)
    {
        if (gByte1A76C8)
        {
            dbCrypt((char*)&sector[i], sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
        IOBuffer1.write(&sector[i], sizeof(sectortype));
        if (gByte1A76C8)
        {
            dbCrypt((char*)&sector[i], sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
        if (sector[i].extra > 0)
        {
            char pBuffer[nXSectorSize];
            BitWriter bitWriter(pBuffer, nXSectorSize);
            XSECTOR* pXSector = &xsector[sector[i].extra];
            bitWriter.write(pXSector->reference, 14);
            bitWriter.write(pXSector->state, 1);
            bitWriter.write(pXSector->busy, 17);
            bitWriter.write(pXSector->data, 16);
            bitWriter.write(pXSector->txID, 10);
            bitWriter.write(pXSector->busyWaveA, 3);
            bitWriter.write(pXSector->busyWaveB, 3);
            bitWriter.write(pXSector->rxID, 10);
            bitWriter.write(pXSector->command, 8);
            bitWriter.write(pXSector->triggerOn, 1);
            bitWriter.write(pXSector->triggerOff, 1);
            bitWriter.write(pXSector->busyTimeA, 12);
            bitWriter.write(pXSector->waitTimeA, 12);
            bitWriter.write(pXSector->restState, 1);
            bitWriter.write(pXSector->interruptable, 1);
            bitWriter.write(pXSector->amplitude, 8);
            bitWriter.write(pXSector->shadeFreq, 8);
            bitWriter.write(pXSector->reTriggerA, 1);
            bitWriter.write(pXSector->reTriggerB, 1);
            bitWriter.write(pXSector->shadePhase, 8);
            bitWriter.write(pXSector->shadeWave, 4);
            bitWriter.write(pXSector->shadeAlways, 1);
            bitWriter.write(pXSector->shadeFloor, 1);
            bitWriter.write(pXSector->shadeCeiling, 1);
            bitWriter.write(pXSector->shadeWalls, 1);
            bitWriter.write(pXSector->shade, 8);
            bitWriter.write(pXSector->panAlways, 1);
            bitWriter.write(pXSector->panFloor, 1);
            bitWriter.write(pXSector->panCeiling, 1);
            bitWriter.write(pXSector->drag, 1);
            bitWriter.write(pXSector->underwater, 1);
            bitWriter.write(pXSector->Depth, 3);
            bitWriter.write(pXSector->panVel, 8);
            bitWriter.write(pXSector->panAngle, 11);
            bitWriter.write(pXSector->unused1, 1);
            bitWriter.write(pXSector->decoupled, 1);
            bitWriter.write(pXSector->triggerOnce, 1);
            bitWriter.write(pXSector->isTriggered, 1);
            bitWriter.write(pXSector->key, 3);
            bitWriter.write(pXSector->triggerPush, 1);
            bitWriter.write(pXSector->triggerVector, 1);
            bitWriter.write(pXSector->triggerReserved, 1);
            bitWriter.write(pXSector->triggerEnter, 1);
            bitWriter.write(pXSector->triggerExit, 1);
            bitWriter.write(pXSector->triggerWallPush, 1);
            bitWriter.write(pXSector->coloredLights, 1);
            bitWriter.write(pXSector->unused2, 1);
            bitWriter.write(pXSector->busyTimeB, 12);
            bitWriter.write(pXSector->waitTimeB, 12);
            bitWriter.write(pXSector->stopOn, 1);
            bitWriter.write(pXSector->stopOff, 1);
            bitWriter.write(pXSector->ceilpal2, 4);
            bitWriter.write(pXSector->offCeilZ, 32);
            bitWriter.write(pXSector->onCeilZ, 32);
            bitWriter.write(pXSector->offFloorZ, 32);
            bitWriter.write(pXSector->onFloorZ, 32);
            bitWriter.write(pXSector->marker0, 16);
            bitWriter.write(pXSector->marker1, 16);
            bitWriter.write(pXSector->crush, 1);
            bitWriter.write(pXSector->ceilXPanFrac, 8);
            bitWriter.write(pXSector->ceilYPanFrac, 8);
            bitWriter.write(pXSector->floorXPanFrac, 8);
            bitWriter.write(pXSector->damageType, 3);
            bitWriter.write(pXSector->floorpal2, 4);
            bitWriter.write(pXSector->floorYPanFrac, 8);
            bitWriter.write(pXSector->locked, 1);
            bitWriter.write(pXSector->windVel, 10);
            bitWriter.write(pXSector->windAng, 11);
            bitWriter.write(pXSector->windAlways, 1);
            bitWriter.write(pXSector->dudeLockout, 1);
            bitWriter.write(pXSector->bobTheta, 11);
            bitWriter.write(pXSector->bobZRange, 5);
            bitWriter.write(pXSector->bobSpeed, 12);
            bitWriter.write(pXSector->bobAlways, 1);
            bitWriter.write(pXSector->bobFloor, 1);
            bitWriter.write(pXSector->bobCeiling, 1);
            bitWriter.write(pXSector->bobRotate, 1);
            IOBuffer1.write(pBuffer, nXSectorSize);
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        if (gByte1A76C8)
        {
            dbCrypt((char*)&wall[i], sizeof(walltype), gMapRev*sizeof(sectortype) | 0x7474614d);
        }
        IOBuffer1.write(&wall[i], sizeof(walltype));
        if (gByte1A76C8)
        {
            dbCrypt((char*)&wall[i], sizeof(walltype), gMapRev*sizeof(sectortype) | 0x7474614d);
        }
        if (wall[i].extra > 0)
        {
            char pBuffer[nXWallSize];
            BitWriter bitWriter(pBuffer, nXWallSize);
            XWALL* pXWall = &xwall[wall[i].extra];
            bitWriter.write(pXWall->reference, 14);
            bitWriter.write(pXWall->state, 1);
            bitWriter.write(pXWall->busy, 17);
            bitWriter.write(pXWall->data, 16);
            bitWriter.write(pXWall->txID, 10);
            bitWriter.write(pXWall->unused1, 6);
            bitWriter.write(pXWall->rxID, 10);
            bitWriter.write(pXWall->command, 8);
            bitWriter.write(pXWall->triggerOn, 1);
            bitWriter.write(pXWall->triggerOff, 1);
            bitWriter.write(pXWall->busyTime, 12);
            bitWriter.write(pXWall->waitTime, 12);
            bitWriter.write(pXWall->restState, 1);
            bitWriter.write(pXWall->interruptable, 1);
            bitWriter.write(pXWall->panAlways, 1);
            bitWriter.write(pXWall->panXVel, 8);
            bitWriter.write(pXWall->panYVel, 8);
            bitWriter.write(pXWall->decoupled, 1);
            bitWriter.write(pXWall->triggerOnce, 1);
            bitWriter.write(pXWall->isTriggered, 1);
            bitWriter.write(pXWall->key, 3);
            bitWriter.write(pXWall->triggerPush, 1);
            bitWriter.write(pXWall->triggerVector, 1);
            bitWriter.write(pXWall->triggerTouch, 1);
            bitWriter.write(pXWall->unused2, 2);
            bitWriter.write(pXWall->xpanFrac, 8);
            bitWriter.write(pXWall->ypanFrac, 8);
            bitWriter.write(pXWall->locked, 1);
            bitWriter.write(pXWall->dudeLockout, 1);
            bitWriter.write(pXWall->unused3, 4);
            bitWriter.write(pXWall->unused4, 32);
            IOBuffer1.write(pBuffer, nXWallSize);
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            if (gByte1A76C8)
            {
                dbCrypt((char*)&sprite[i], sizeof(spritetype), gMapRev*sizeof(spritetype) | 'ttaM');
            }
            IOBuffer1.write(&sprite[i], sizeof(spritetype));
            if (gByte1A76C8)
            {
                dbCrypt((char*)&sprite[i], sizeof(spritetype), gMapRev*sizeof(spritetype) | 'ttaM');
            }
            if (sprite[i].extra > 0)
            {
                char pBuffer[nXSpriteSize];
                BitWriter bitWriter(pBuffer, nXSpriteSize);
                XSPRITE* pXSprite = &xsprite[sprite[i].extra];
                bitWriter.write(pXSprite->reference, 14);
                bitWriter.write(pXSprite->state, 1);
                bitWriter.write(pXSprite->busy, 17);
                bitWriter.write(pXSprite->txID, 10);
                bitWriter.write(pXSprite->rxID, 10);
                bitWriter.write(pXSprite->command, 8);
                bitWriter.write(pXSprite->triggerOn, 1);
                bitWriter.write(pXSprite->triggerOff, 1);
                bitWriter.write(pXSprite->wave, 2);
                bitWriter.write(pXSprite->busyTime, 12);
                bitWriter.write(pXSprite->waitTime, 12);
                bitWriter.write(pXSprite->restState, 1);
                bitWriter.write(pXSprite->interruptable, 1);
                bitWriter.write(pXSprite->unused1, 2);
                bitWriter.write(pXSprite->respawnPending, 2);
                bitWriter.write(pXSprite->unused2, 1);
                bitWriter.write(pXSprite->lT, 1);
                bitWriter.write(pXSprite->dropItem, 8);
                bitWriter.write(pXSprite->decoupled, 1);
                bitWriter.write(pXSprite->triggerOnce, 1);
                bitWriter.write(pXSprite->isTriggered, 1);
                bitWriter.write(pXSprite->key, 3);
                bitWriter.write(pXSprite->triggerPush, 1);
                bitWriter.write(pXSprite->triggerVector, 1);
                bitWriter.write(pXSprite->triggerImpact, 1);
                bitWriter.write(pXSprite->triggerPickup, 1);
                bitWriter.write(pXSprite->triggerTouch, 1);
                bitWriter.write(pXSprite->triggerSight, 1);
                bitWriter.write(pXSprite->triggerProximity, 1);
                bitWriter.write(pXSprite->unused3, 2);
                bitWriter.write(pXSprite->lSkill, 5);
                bitWriter.write(pXSprite->lS, 1);
                bitWriter.write(pXSprite->lB, 1);
                bitWriter.write(pXSprite->lC, 1);
                bitWriter.write(pXSprite->dudeLockout, 1);
                bitWriter.write(pXSprite->data1, 16);
                bitWriter.write(pXSprite->data2, 16);
                bitWriter.write(pXSprite->data3, 16);
                bitWriter.write(pXSprite->goalAng, 11);
                bitWriter.write(pXSprite->dodgeDir, 2);
                bitWriter.write(pXSprite->locked, 1);
                bitWriter.write(pXSprite->medium, 2);
                bitWriter.write(pXSprite->respawn, 2);
                bitWriter.write(pXSprite->data4, 16);
                bitWriter.write(pXSprite->unused4, 6);
                bitWriter.write(pXSprite->lockMsg, 8);
                bitWriter.write(pXSprite->health, 12);
                bitWriter.write(pXSprite->dudeDeaf, 1);
                bitWriter.write(pXSprite->dudeAmbush, 1);
                bitWriter.write(pXSprite->dudeGuard, 1);
                bitWriter.write(pXSprite->dudeFlag4, 1);
                bitWriter.write(pXSprite->target, 16);
                bitWriter.write(pXSprite->targetX, 32);
                bitWriter.write(pXSprite->targetY, 32);
                bitWriter.write(pXSprite->targetZ, 32);
                bitWriter.write(pXSprite->burnTime, 16);
                bitWriter.write(pXSprite->burnSource, 16);
                bitWriter.write(pXSprite->height, 16);
                bitWriter.write(pXSprite->stateTimer, 16);
                IOBuffer1.write(pBuffer, nXSpriteSize);
            }
        }
    }
    unsigned int nCRC = crc32once((unsigned char*)pData, nSize-4);
    IOBuffer1.write(&nCRC, 4);
		
	rename(sMapExt, sBakExt);
    int nHandle = Bopen(sMapExt, BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY, BS_IREAD|BS_IWRITE);
    if (nHandle == -1)
    {
        buildprintf("Couldn't open \"%s\" for writing: %s\n", sMapExt, strerror(errno));
        Bfree(pData);
        return -1;
    }
    if (Bwrite(nHandle, pData, nSize) != nSize)
    {
        buildprintf("Couldn't write to \"%s\": %s\n", sMapExt, strerror(errno));
        Bclose(nHandle);
        Bfree(pData);
        return -1;
    }
    Bclose(nHandle);
    Bfree(pData);
	
	if (gCmtPrefs.enabled)
	{
		char cmtfile[_MAX_PATH];
		sprintf(cmtfile, sMapExt);
		ChangeExtension(cmtfile, kCommentExt);
		if (fileExists(cmtfile))
		{
			fileAttrSetWrite(cmtfile);
			unlink(cmtfile);
		}
		
		gCommentMgr.SetCRC(nCRC);
		if (gCommentMgr.commentsCount > 0)
		{
			IniFile* pFile = new IniFile(cmtfile);
			gCommentMgr.SaveToIni(pFile);
			delete pFile;
		}
	}
	
    return 0;

}

