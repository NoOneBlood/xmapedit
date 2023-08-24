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

#include "mapcmt.h"
#include "xmpconf.h"
#include "xmpstub.h"
#include "xmpror.h"
#include "xmpmisc.h"

#include "gui.h"

int gSuppMapVersions[] = { 4, 6, 7, 8, 9, };
bool gModernMap = false, gCustomSkyBits = false;
int gMapRev = 0, gSkyCount = 1;

unsigned short gStatCount[kMaxStatus + 1];
unsigned short nextXSprite[kMaxXSprites];
unsigned short nextXSector[kMaxXSectors];
unsigned short nextXWall[kMaxXWalls];

XSPRITE xsprite[kMaxXSprites];
XSECTOR xsector[kMaxXSectors];
XWALL xwall[kMaxXWalls];

int xvel[kMaxSprites];
int yvel[kMaxSprites];
int zvel[kMaxSprites];

short numxsprites = 0;
short numxsectors = 0;
short numxwalls   = 0;

int qdeletesprite(short nSprite) { return DeleteSprite(nSprite); }
int qchangespritesect(short nSprite, short nSector) { return ChangeSpriteSect(nSprite, nSector); }
int qchangespritestat(short nSprite, short nStatus) { return ChangeSpriteStat(nSprite, nStatus); }
int qinsertsprite(short nSector, short nStat) { return InsertSprite(nSector, nStat); }
void qinitspritelists(void) // Replace
{
	int i;
	memset(headspritesect, -1, sizeof(headspritesect));
	memset(headspritestat, -1, sizeof(headspritestat));
	memset(gStatCount, 		0, sizeof(gStatCount));
	for (i = 0; i < kMaxSprites; i++)
	{
		sprite[i].sectnum = sprite[i].index = -1;
		InsertSpriteStat(i, kMaxStatus);
	}
}

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
				numxsprites--;
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
				numxwalls--;
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
				numxsectors--;
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

int dbCheckMap(IOBuffer* pIo, int* pSuppVerDB, int DBLen, BOOL* bloodMap)
{
	#define kSiz sizeof(kBloodMapSig)

	int i, ver = 0;
	char tmp[kSiz]; memset(tmp, 0, sizeof(tmp));
	*bloodMap = FALSE;

	pIo->rewind();
	if (pIo->nRemain <= kSiz)
		return -2;

	pIo->read(tmp, kSiz-1);
	// check if this is a Blood map via signature
	if (memcmp(tmp, kBloodMapSig, kSiz) == 0)
	{
		if (pIo->nRemain < 4)
			return -2;

		pIo->read(&i, 4);
		i = i & 0xFF00;
		if (i == (0x0603 & 0xFF00)) ver = 6;
		else if (i == (0x0700 & 0xFF00))
			ver = 7;

		if (ver)
			*bloodMap = TRUE;
	}
	// probably a generic BUILD map file
	else
	{
		pIo->rewind();
		if (pIo->nRemain < 4)
			return -2;

		pIo->read(&ver, 4);
	}

	if (!pSuppVerDB) return ver;
	else
	{
		// check supported versions
		while(DBLen--)
		{
			if (ver == pSuppVerDB[DBLen])
				return ver;
		}
	}

	return -3;
}

int dbLoadBuildMap(IOBuffer* pIo) {

	int i, len, ver = 0;
	SECTOR4 sector4; WALL4 wall4; SPRITE4 sprite4;
	SECTOR6 sector6; WALL6 wall6; SPRITE6 sprite6;

	gModernMap          = FALSE;
	gCustomSkyBits      = TRUE;
	gSkyCount           = 1;
	pskybits            = 0;
	pskyoff[0]          = 0;
	parallaxtype        = 0;
	visibility          = 0;
	parallaxvisibility  = 0;
	dbInit(); // this calling initspritelists() too

	pIo->rewind();
	pIo->read(&ver,             4);                             // version
	pIo->read(&startposx,       4);                             // x-pos
	pIo->read(&startposy,       4);                             // y-pos
	pIo->read(&startposz,       4);                             // z-pos
	pIo->read(&startang,        2);                             // angle
	pIo->read(&startsectnum,    2);                             // start sector

	switch(ver) {
		case 7L:
		case 8L:
		case 9L:
			pIo->read(&numsectors,  2);                         // read numsectors
			pIo->read(sector, sizeof(sectortype)*numsectors);   // fill the array
			pIo->read(&numwalls,    2);                         // read numwalls
			pIo->read(wall, sizeof(walltype)*numwalls);         // fill the array
			pIo->read(&numsprites,  2);                         // read numsprites
			for (i =0; i < numsprites; i++)                     // insert sprites
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite[i], sizeof(spritetype));
				InsertSpriteSect(i, sprite[i].sectnum);
				InsertSpriteStat(i, sprite[i].statnum);
				sprite[i].index     = i;
			}
			break;
		case 4L:
			// this is based on Ken's convmaps
			pIo->read(&numsectors,  2);                         // read numsectors
			pIo->read(&numwalls,    2);                         // read numwalls
			pIo->read(&numsprites,  2);                         // read numsprites
			for (i = 0; i < numsectors; i++)                    // convert each sector
			{
				pIo->read(&sector4, sizeof(SECTOR4));
				sector[i].wallptr           = sector4.wallptr;
				sector[i].wallnum           = sector4.wallnum;
				sector[i].ceilingshade      = sector4.ceilingshade;
				sector[i].ceilingz          = sector4.ceilingz;
				sector[i].ceilingpicnum     = sector4.ceilingpicnum;
				sector[i].ceilingslope      = 0;
				sector[i].floorshade        = sector4.floorshade;
				sector[i].floorz            = sector4.floorz;
				sector[i].floorpicnum       = sector4.floorpicnum;
				sector[i].floorslope        = 0;
				sector[i].type              = (short)(sector4.tag & 65535);
				sector[i].hitag             = (short)(sector4.tag >> 16);
				sector[i].ceilingstat       = sector4.ceilingstat;
				sector[i].floorstat         = sector4.floorstat;
			}
			for (i = 0; i < numwalls; i++)                          // convert each wall
			{
				pIo->read(&wall4, sizeof(WALL4));
				wall[i].x               = wall4.x;
				wall[i].y               = wall4.y;
				wall[i].point2          = wall4.point2;
				wall[i].nextsector      = wall4.nextsector1;
				wall[i].nextwall        = wall4.nextwall1;
				wall[i].picnum          = wall4.picnum;
				wall[i].overpicnum      = wall4.overpicnum;
				wall[i].shade           = wall4.shade;
				wall[i].pal             = sector[sectorofwall((short)i)].floorpal;
				wall[i].cstat           = wall4.cstat;
				wall[i].xrepeat         = wall4.xrepeat;
				wall[i].yrepeat         = wall4.yrepeat;
				wall[i].xpanning        = wall4.xpanning;
				wall[i].ypanning        = wall4.ypanning;
				wall[i].type            = (short)(wall4.tag & 65535);
				wall[i].hitag           = (short)(wall4.tag >> 16);
			}
			for (i = 0; i < numsprites; i++)                        // convert each sprite
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite4, sizeof(SPRITE4));
				InsertSpriteSect(i, sprite4.sectnum);
				InsertSpriteStat(i, sprite4.statnum);
				sprite[i].index         = i;
				sprite[i].x             = sprite4.x;
				sprite[i].y             = sprite4.y;
				sprite[i].z             = sprite4.z;
				sprite[i].cstat         = sprite4.cstat;
				sprite[i].shade         = sprite4.shade;
				sprite[i].xrepeat       = sprite4.xrepeat;
				sprite[i].yrepeat       = sprite4.yrepeat;
				sprite[i].pal           = 0;
				sprite[i].xoffset       = 0;
				sprite[i].yoffset       = 0;
				sprite[i].clipdist      = 32;
				sprite[i].picnum        = sprite4.picnum;
				sprite[i].ang           = sprite4.ang;
				sprite[i].yvel          = sprite4.yvel;
				sprite[i].zvel          = sprite4.zvel;
				sprite[i].owner         = sprite4.owner;
				sprite[i].sectnum       = sprite4.sectnum;
				sprite[i].statnum       = sprite4.statnum;
				sprite[i].type          = (short)(sprite4.tag & 65535);
				sprite[i].flags         = (short)(sprite4.tag >> 16);

				if ((sprite[i].cstat & kSprRelMask) == 48)
					sprite[i].cstat &= ~48;
			}
			break;
		case 6L:
			// this is based on eduke32 v6->v7 conversion
			pIo->read(&numsectors, 2);                              // read numsectors
			for (i = 0; i < numsectors; i++)                        // convert each one
			{
				pIo->read(&sector6, sizeof(SECTOR6));
				sector[i].wallptr           = sector6.wallptr;
				sector[i].wallnum           = sector6.wallnum;
				sector[i].ceilingstat       = sector6.ceilingstat;
				sector[i].ceilingz          = sector6.ceilingz;
				sector[i].ceilingpicnum     = sector6.ceilingpicnum;
				sector[i].ceilingshade      = sector6.ceilingshade;
				sector[i].ceilingpal        = sector6.ceilingpal;
				sector[i].ceilingxpanning   = sector6.ceilingxpanning;
				sector[i].ceilingypanning   = sector6.ceilingypanning;
				sector[i].floorstat         = sector6.floorstat;
				sector[i].floorz            = sector6.floorz;
				sector[i].floorpicnum       = sector6.floorpicnum;
				sector[i].floorshade        = sector6.floorshade;
				sector[i].floorpal          = sector6.floorpal;
				sector[i].floorxpanning     = sector6.floorxpanning;
				sector[i].floorypanning     = sector6.floorypanning;
				sector[i].visibility        = sector6.visibility;
				sector[i].type              = sector6.lotag;
				sector[i].hitag             = sector6.hitag;
				//sector[i].extra               = sector6.extra;

				sector[i].ceilingslope = (short)max(min(((int)sector6.ceilingheinum) << 5, 32767), -32768);
				if ((sector6.ceilingstat & 2) == 0)
					sector[i].ceilingslope = 0;

				sector[i].floorslope = (short)max(min(((int)sector6.floorheinum) << 5, 32767), -32768);
				if ((sector6.floorstat & 2) == 0)
					sector[i].floorslope = 0;
			}

			pIo->read(&numwalls, 2);                                // read numwalls
			for (i = 0; i < numwalls; i++)                          // convert each one
			{
				pIo->read(&wall6, sizeof(WALL6));
				wall[i].x           = wall6.x;
				wall[i].y           = wall6.y;
				wall[i].point2      = wall6.point2;
				wall[i].nextwall    = wall6.nextwall;
				wall[i].nextsector  = wall6.nextsector;
				wall[i].cstat       = wall6.cstat;
				wall[i].picnum      = wall6.picnum;
				wall[i].overpicnum  = wall6.overpicnum;
				wall[i].shade       = wall6.shade;
				wall[i].pal         = wall6.pal;
				wall[i].xrepeat     = wall6.xrepeat;
				wall[i].yrepeat     = wall6.yrepeat;
				wall[i].xpanning    = wall6.xpanning;
				wall[i].ypanning    = wall6.ypanning;
				wall[i].type        = wall6.lotag;
				wall[i].hitag       = wall6.hitag;
				//wall[i].extra     = wall6.extra;
			}

			pIo->read(&numsprites, sizeof(short));                      // read numsprites
			for (i = 0; i < numsprites; i++)                            // convert each one
			{
				RemoveSpriteStat(i);
				pIo->read(&sprite6, sizeof(SPRITE6));
				InsertSpriteSect(i, sprite6.sectnum);
				InsertSpriteStat(i, sprite6.statnum);
				sprite[i].index     = i;
				sprite[i].x         = sprite6.x;
				sprite[i].y         = sprite6.y;
				sprite[i].z         = sprite6.z;
				sprite[i].cstat     = sprite6.cstat;
				sprite[i].picnum    = sprite6.picnum;
				sprite[i].shade     = sprite6.shade;
				sprite[i].pal       = sprite6.pal;
				sprite[i].clipdist  = sprite6.clipdist;
				sprite[i].xrepeat   = sprite6.xrepeat;
				sprite[i].yrepeat   = sprite6.yrepeat;
				sprite[i].xoffset   = sprite6.xoffset;
				sprite[i].yoffset   = sprite6.yoffset;
				sprite[i].owner     = sprite6.owner;
				sprite[i].sectnum   = sprite6.sectnum;
				sprite[i].statnum   = sprite6.statnum;
				sprite[i].ang       = sprite6.ang;
				sprite[i].yvel      = sprite6.yvel;
				sprite[i].zvel      = sprite6.zvel;
				sprite[i].type      = sprite6.lotag;
				sprite[i].flags     = sprite6.hitag;
				//sprite[i].extra       = sprite6.extra;

				if ((sprite[i].cstat & kSprRelMask) == kSprSloped)
					sprite[i].cstat &= ~kSprSloped;
			}
			break;
	}
	
	eraseExtra();
	posx = startposx;
	posy = startposy;
	posz = startposz;
	ang  = startang;
	return ver;
}

int dbLoadMap(IOBuffer* pIo, char* cmtpath)
{
	int16_t tpskyoff[LENGTH(pskyoff)];
	char temp[kXSectorDiskSize + kXSpriteDiskSize + kXWallDiskSize];
	int i, nSkySize, nKey, oExtra, nExtra; unsigned int nCRC;
	
	BOOL ver7, loadComments = (cmtpath != NULL);
	BLMMAGIC magic; BLMHEADER_MAIN header; BLMHEADER_EXTRA extra;
	XWALL* pXWall;  XSECTOR* pXSect;  XSPRITE* pXSpr;

	pIo->seek(4, SEEK_END); pIo->read(&nCRC, 4);
	
	#if B_BIG_ENDIAN == 1
	nCRC = B_LITTLE32(nCRC);
	#endif
	
	pIo->rewind();
	if (crc32once(pIo->pBuffer, pIo->nTotal - 4) != nCRC)
		ThrowError("Map File does not match CRC");
	
	pIo->read(&magic, sizeof(magic));
	if (memcmp(magic.sign, kBloodMapSig, 4) != 0)
		ThrowError("Map file is not Blood format");
	
	#if B_BIG_ENDIAN == 1
	magic.version = B_LITTLE16(magic.version);
	#endif

	ver7 = ((magic.version & 0xFF00) == (0x0700 & 0xFF00));
	if (!ver7 && (magic.version & 0xFF00) != (0x0603 & 0xFF00))
		ThrowError("Map file is wrong version");
		
	pIo->read(&header, sizeof(header));
	if (ver7)
		dbCrypt((char*)&header, sizeof(header), kMattID2);

	startposx		= B_LITTLE32(header.startX);
	startposy		= B_LITTLE32(header.startY);
	startposz		= B_LITTLE32(header.startZ);
	startang        = B_LITTLE16(header.startA);
	startsectnum    = B_LITTLE16(header.startS);
	pskybits        = B_LITTLE16(header.skyBits);
	visibility      = B_LITTLE32(header.visibility);
	gMapRev         = B_LITTLE32(header.revision);
	numsectors      = B_LITTLE16(header.numsectors);
	numwalls        = B_LITTLE16(header.numwalls);
	numsprites      = B_LITTLE16(header.numsprites);
	parallaxtype    = header.skyType;
	
	gSkyCount       = ClipHigh(1 << pskybits, MAXPSKYTILES);
	nSkySize        = gSkyCount*sizeof(tpskyoff[0]);
	boardWidth		= MET2PIX(384);
	boardHeight 	= MET2PIX(384);
	gFogMode 		&= ~kXmpFlagFog;
	
	if (ver7)
	{
		pIo->read(&extra, sizeof(extra));
		dbCrypt((char*)&extra, sizeof(extra), numwalls);
	
		// XMAPEDIT specific info
		if (memcmp(kXMPHeadSig, extra.xmpsign, sizeof(kXMPHeadSig)-1) == 0)
		{
			if (irngok(extra.xmpheadver, kXMPHeadVer1, kXMPHeadVer2))
			{
				gCustomSkyBits = (bool)(extra.xmpMapFlags & kXmpFlagCustomSkyBits);
				if (extra.xmpMapFlags & kXmpFlagFog)
					gFogMode |= kXmpFlagFog;
				
				if (extra.xmpheadver == kXMPHeadVer2)
				{
					boardWidth	= MET2PIX(extra.xmpBoardWidth);
					boardHeight = MET2PIX(extra.xmpBoardHeight);
				}
			}
		}
		
		pIo->read(tpskyoff, nSkySize);
		dbCrypt((char*)tpskyoff, nSkySize, nSkySize);
	}
	else
	{
		extra.xsecSiz = kXSectorDiskSize;
		extra.xsprSiz = kXSpriteDiskSize;
		extra.xwalSiz = kXWallDiskSize;
		pIo->read(tpskyoff, nSkySize);
	}

	#if B_BIG_ENDIAN == 1
	extra.xsecSiz = B_LITTLE32(extra.xsecSiz);
	extra.xsprSiz = B_LITTLE32(extra.xsprSiz);
	extra.xwalSiz = B_LITTLE32(extra.xwalSiz);
	#endif
	
	i = 0;
	while (i < gSkyCount)
		pskyoff[i] = B_LITTLE16(tpskyoff[i++]);
	
	gModernMap  = (ver7 && (magic.version & 0x00FF) == 0x001); // do the map use modern features?
	numxsprites = numxwalls = numxsectors = 0;

	dbInit();                   // this calling initspritelists() too
	gCommentMgr.DeleteAll();    // erase all the comments that may left from the previous map

	if (loadComments)
		loadComments = (BOOL)(gCommentMgr.LoadFromIni(cmtpath) > 0);

	nKey = gMapRev*sizeof(sectortype);
	for (i = 0; i < numsectors; i++)
	{
		sectortype *pSect = &sector[i];
		pIo->read(pSect, sizeof(sectortype));
		if (ver7)
			dbCrypt((char*)pSect, sizeof(sectortype), nKey);

		#if B_BIG_ENDIAN == 1
		pSect->wallptr 			= B_LITTLE16(pSect->wallptr);
		pSect->wallnum 			= B_LITTLE16(pSect->wallnum);
		pSect->ceilingz 		= B_LITTLE32(pSect->ceilingz);
		pSect->floorz 			= B_LITTLE32(pSect->floorz);
		pSect->ceilingstat		= B_LITTLE16(pSect->ceilingstat);
		pSect->floorstat 		= B_LITTLE16(pSect->floorstat);
		pSect->ceilingpicnum	= B_LITTLE16(pSect->ceilingpicnum);
		pSect->ceilingheinum 	= B_LITTLE16(pSect->ceilingheinum);
		pSect->floorpicnum 		= B_LITTLE16(pSect->floorpicnum);
		pSect->floorheinum 		= B_LITTLE16(pSect->floorheinum);
		pSect->type 			= B_LITTLE16(pSect->type);
		pSect->hitag 			= B_LITTLE16(pSect->hitag);
		pSect->extra 			= B_LITTLE16(pSect->extra);
		#endif

		if (pSect->extra <= 0)
			continue;

		oExtra = pSect->extra;
		nExtra = dbInsertXSector(i);
		pXSect =& xsector[nExtra];

		if (loadComments) // rebind from old xsector to a newly created
			gCommentMgr.RebindMatching(OBJ_SECTOR, oExtra, OBJ_SECTOR, nExtra, TRUE);

		pIo->read(temp, extra.xsecSiz);
		// -----------------------------------------------------------------------
		BitReader BR(temp, extra.xsecSiz);
		pXSect->reference = BR.readSigned(14);          pXSect->state = BR.readUnsigned(1);
		pXSect->busy = BR.readUnsigned(17);             pXSect->data = BR.readUnsigned(16);
		pXSect->txID = BR.readUnsigned(10);             pXSect->busyWaveA = BR.readUnsigned(3);
		pXSect->busyWaveB = BR.readUnsigned(3);         pXSect->rxID = BR.readUnsigned(10);
		pXSect->command = BR.readUnsigned(8);           pXSect->triggerOn = BR.readUnsigned(1);
		pXSect->triggerOff = BR.readUnsigned(1);        pXSect->busyTimeA = BR.readUnsigned(12);
		pXSect->waitTimeA = BR.readUnsigned(12);        pXSect->restState = BR.readUnsigned(1);
		pXSect->interruptable = BR.readUnsigned(1);     pXSect->amplitude = BR.readSigned(8);
		pXSect->shadeFreq = BR.readUnsigned(8);         pXSect->reTriggerA = BR.readUnsigned(1);
		pXSect->reTriggerB = BR.readUnsigned(1);        pXSect->shadePhase = BR.readUnsigned(8);
		pXSect->shadeWave = BR.readUnsigned(4);         pXSect->shadeAlways = BR.readUnsigned(1);
		pXSect->shadeFloor = BR.readUnsigned(1);        pXSect->shadeCeiling = BR.readUnsigned(1);
		pXSect->shadeWalls = BR.readUnsigned(1);        pXSect->shade = BR.readSigned(8);
		pXSect->panAlways = BR.readUnsigned(1);         pXSect->panFloor = BR.readUnsigned(1);
		pXSect->panCeiling = BR.readUnsigned(1);        pXSect->drag = BR.readUnsigned(1);
		pXSect->underwater = BR.readUnsigned(1);        pXSect->Depth = BR.readUnsigned(3);
		pXSect->panVel = BR.readUnsigned(8);            pXSect->panAngle = BR.readUnsigned(11);
		pXSect->unused1 = BR.readUnsigned(1);           pXSect->decoupled = BR.readUnsigned(1);
		pXSect->triggerOnce = BR.readUnsigned(1);       pXSect->isTriggered = BR.readUnsigned(1);
		pXSect->key = BR.readUnsigned(3);               pXSect->triggerPush = BR.readUnsigned(1);
		pXSect->triggerVector = BR.readUnsigned(1);     pXSect->triggerReserved = BR.readUnsigned(1);
		pXSect->triggerEnter = BR.readUnsigned(1);      pXSect->triggerExit = BR.readUnsigned(1);
		pXSect->triggerWallPush = BR.readUnsigned(1);   pXSect->coloredLights = BR.readUnsigned(1);
		pXSect->unused2 = BR.readUnsigned(1);           pXSect->busyTimeB = BR.readUnsigned(12);
		pXSect->waitTimeB = BR.readUnsigned(12);        pXSect->stopOn = BR.readUnsigned(1);
		pXSect->stopOff = BR.readUnsigned(1);           pXSect->ceilpal2 = BR.readUnsigned(4);
		pXSect->offCeilZ = BR.readSigned(32);           pXSect->onCeilZ = BR.readSigned(32);
		pXSect->offFloorZ = BR.readSigned(32);          pXSect->onFloorZ = BR.readSigned(32);
		pXSect->marker0 = BR.readUnsigned(16);          pXSect->marker1 = BR.readUnsigned(16);
		pXSect->crush = BR.readUnsigned(1);             pXSect->ceilXPanFrac = BR.readUnsigned(8);
		pXSect->ceilYPanFrac = BR.readUnsigned(8);      pXSect->floorXPanFrac = BR.readUnsigned(8);
		pXSect->damageType = BR.readUnsigned(3);        pXSect->floorpal2 = BR.readUnsigned(4);
		pXSect->floorYPanFrac = BR.readUnsigned(8);     pXSect->locked = BR.readUnsigned(1);
		pXSect->windVel = BR.readUnsigned(10);          pXSect->windAng = BR.readUnsigned(11);
		pXSect->windAlways = BR.readUnsigned(1);        pXSect->dudeLockout = BR.readUnsigned(1);
		pXSect->bobTheta = BR.readUnsigned(11);         pXSect->bobZRange = BR.readUnsigned(5);
		pXSect->bobSpeed = BR.readSigned(12);           pXSect->bobAlways = BR.readUnsigned(1);
		pXSect->bobFloor = BR.readUnsigned(1);          pXSect->bobCeiling = BR.readUnsigned(1);
		pXSect->bobRotate = BR.readUnsigned(1);         pXSect->reference = i;
		pXSect->busy = pXSect->state<<16;
	}

	nKey = (gMapRev*sizeof(sectortype)) | kMattID2;
	for (i = 0; i < numwalls; i++)
	{
		walltype *pWall = &wall[i];
		pIo->read(pWall, sizeof(walltype));
		if (ver7)
			dbCrypt((char*)pWall, sizeof(walltype), nKey);

		#if B_BIG_ENDIAN == 1
		pWall->x 			= B_LITTLE32(pWall->x);
		pWall->y 			= B_LITTLE32(pWall->y);
		pWall->point2 		= B_LITTLE16(pWall->point2);
		pWall->nextwall 	= B_LITTLE16(pWall->nextwall);
		pWall->nextsector 	= B_LITTLE16(pWall->nextsector);
		pWall->cstat 		= B_LITTLE16(pWall->cstat);
		pWall->picnum 		= B_LITTLE16(pWall->picnum);
		pWall->overpicnum	= B_LITTLE16(pWall->overpicnum);
		pWall->type 		= B_LITTLE16(pWall->type);
		pWall->hitag 		= B_LITTLE16(pWall->hitag);
		pWall->extra 		= B_LITTLE16(pWall->extra);
		#endif

		if (pWall->extra <= 0)
			continue;

		oExtra   = pWall->extra;
		nExtra   = dbInsertXWall(i);
		pXWall   =& xwall[nExtra];

		if (loadComments) // rebind from old xwall to a newly created
			gCommentMgr.RebindMatching(OBJ_WALL, oExtra, OBJ_WALL, nExtra, TRUE);

		pIo->read(temp, extra.xwalSiz);
		// -----------------------------------------------------------------------
		BitReader BR(temp, extra.xwalSiz);
		pXWall->reference = BR.readSigned(14);          pXWall->state = BR.readUnsigned(1);
		pXWall->busy = BR.readUnsigned(17);             pXWall->data = BR.readSigned(16);
		pXWall->txID = BR.readUnsigned(10);             pXWall->unused1 = BR.readUnsigned(6);
		pXWall->rxID = BR.readUnsigned(10);             pXWall->command = BR.readUnsigned(8);
		pXWall->triggerOn = BR.readUnsigned(1);         pXWall->triggerOff = BR.readUnsigned(1);
		pXWall->busyTime = BR.readUnsigned(12);         pXWall->waitTime = BR.readUnsigned(12);
		pXWall->restState = BR.readUnsigned(1);         pXWall->interruptable = BR.readUnsigned(1);
		pXWall->panAlways = BR.readUnsigned(1);         pXWall->panXVel = BR.readSigned(8);
		pXWall->panYVel = BR.readSigned(8);             pXWall->decoupled = BR.readUnsigned(1);
		pXWall->triggerOnce = BR.readUnsigned(1);       pXWall->isTriggered = BR.readUnsigned(1);
		pXWall->key = BR.readUnsigned(3);               pXWall->triggerPush = BR.readUnsigned(1);
		pXWall->triggerVector = BR.readUnsigned(1);     pXWall->triggerTouch = BR.readUnsigned(1);
		pXWall->unused2 = BR.readUnsigned(2);           pXWall->xpanFrac = BR.readUnsigned(8);
		pXWall->ypanFrac = BR.readUnsigned(8);          pXWall->locked = BR.readUnsigned(1);
		pXWall->dudeLockout = BR.readUnsigned(1);       pXWall->unused3 = BR.readUnsigned(4);
		pXWall->unused4 = BR.readUnsigned(32);          pXWall->reference = i;
		pXWall->busy = pXWall->state << 16;
	}

	nKey = (gMapRev*sizeof(spritetype)) | kMattID2;
	for (i = 0; i < numsprites; i++)
	{
		RemoveSpriteStat(i);
		spritetype *pSpr = &sprite[i];
		pIo->read(pSpr, sizeof(spritetype));
		if (ver7)
			dbCrypt((char*)pSpr, sizeof(spritetype), nKey);

		#if B_BIG_ENDIAN == 1
		pSpr->x 			= B_LITTLE32(pSpr->x);
		pSpr->y 			= B_LITTLE32(pSpr->y);
		pSpr->z 			= B_LITTLE32(pSpr->z);
		pSpr->cstat 		= B_LITTLE16(pSpr->cstat);
		pSpr->picnum 		= B_LITTLE16(pSpr->picnum);
		pSpr->sectnum 		= B_LITTLE16(pSpr->sectnum);
		pSpr->statnum 		= B_LITTLE16(pSpr->statnum);
		pSpr->ang 			= B_LITTLE16(pSpr->ang);
		pSpr->owner 		= B_LITTLE16(pSpr->owner);
		pSpr->index 		= B_LITTLE16(pSpr->index);
		pSpr->yvel 			= B_LITTLE16(pSpr->yvel);
		pSpr->inittype 		= B_LITTLE16(pSpr->inittype);
		pSpr->type 			= B_LITTLE16(pSpr->type);
		pSpr->flags 		= B_LITTLE16(pSpr->hitag);
		pSpr->extra 		= B_LITTLE16(pSpr->extra);
		#endif

		InsertSpriteSect(i, sprite[i].sectnum);
		InsertSpriteStat(i, sprite[i].statnum);
		
		if (loadComments) // rebind from old sprite index to a newly created
			gCommentMgr.RebindMatching(OBJ_SPRITE, pSpr->index, OBJ_SPRITE, i, TRUE);

		pSpr->index = i; // set new sprite index after
		
		// remove spin (sloped) sprite cstat for older map versions
		if (!ver7 && (pSpr->cstat & kSprRelMask) == kSprSloped)
			pSpr->cstat &= ~kSprSloped;
		
		if (pSpr->extra <= 0)
			continue;

		nExtra = dbInsertXSprite(i);
		pXSpr  =& xsprite[nExtra];

		pIo->read(temp, extra.xsprSiz);
		// -----------------------------------------------------------------------
		BitReader BR(temp, extra.xsprSiz);
		pXSpr->reference = BR.readSigned(14);           pXSpr->state = BR.readUnsigned(1);
		pXSpr->busy = BR.readUnsigned(17);              pXSpr->txID = BR.readUnsigned(10);
		pXSpr->rxID = BR.readUnsigned(10);              pXSpr->command = BR.readUnsigned(8);
		pXSpr->triggerOn = BR.readUnsigned(1);          pXSpr->triggerOff = BR.readUnsigned(1);
		pXSpr->wave = BR.readUnsigned(2);               pXSpr->busyTime = BR.readUnsigned(12);
		pXSpr->waitTime = BR.readUnsigned(12);          pXSpr->restState = BR.readUnsigned(1);
		pXSpr->interruptable = BR.readUnsigned(1);      pXSpr->unused1 = BR.readUnsigned(2);
		pXSpr->respawnPending = BR.readUnsigned(2);     pXSpr->unused2 = BR.readUnsigned(1);
		pXSpr->lT = BR.readUnsigned(1);                 pXSpr->dropItem = BR.readUnsigned(8);
		pXSpr->decoupled = BR.readUnsigned(1);          pXSpr->triggerOnce = BR.readUnsigned(1);
		pXSpr->isTriggered = BR.readUnsigned(1);        pXSpr->key = BR.readUnsigned(3);
		pXSpr->triggerPush = BR.readUnsigned(1);        pXSpr->triggerVector = BR.readUnsigned(1);
		pXSpr->triggerImpact = BR.readUnsigned(1);      pXSpr->triggerPickup = BR.readUnsigned(1);
		pXSpr->triggerTouch = BR.readUnsigned(1);       pXSpr->triggerSight = BR.readUnsigned(1);
		pXSpr->triggerProximity = BR.readUnsigned(1);   pXSpr->unused3 = BR.readUnsigned(2);
		pXSpr->lSkill = BR.readUnsigned(5);             pXSpr->lS = BR.readUnsigned(1);
		pXSpr->lB = BR.readUnsigned(1);                 pXSpr->lC = BR.readUnsigned(1);
		pXSpr->dudeLockout = BR.readUnsigned(1);        pXSpr->data1 = BR.readSigned(16);
		pXSpr->data2 = BR.readSigned(16);               pXSpr->data3 = BR.readSigned(16);
		pXSpr->goalAng = BR.readUnsigned(11);           pXSpr->dodgeDir = BR.readSigned(2);
		pXSpr->locked = BR.readUnsigned(1);             pXSpr->medium = BR.readUnsigned(2);
		pXSpr->respawn = BR.readUnsigned(2);            pXSpr->data4 = BR.readUnsigned(16);
		pXSpr->unused4 = BR.readUnsigned(6);            pXSpr->lockMsg = BR.readUnsigned(8);
		pXSpr->health = BR.readUnsigned(12);            pXSpr->dudeDeaf = BR.readUnsigned(1);
		pXSpr->dudeAmbush = BR.readUnsigned(1);         pXSpr->dudeGuard = BR.readUnsigned(1);
		pXSpr->dudeFlag4 = BR.readUnsigned(1);          pXSpr->target = BR.readSigned(16);
		pXSpr->targetX = BR.readSigned(32);             pXSpr->targetY = BR.readSigned(32);
		pXSpr->targetZ = BR.readSigned(32);             pXSpr->burnTime = BR.readUnsigned(16);
		pXSpr->burnSource = BR.readSigned(16);          pXSpr->height = BR.readUnsigned(16);
		pXSpr->stateTimer = BR.readUnsigned(16);        pXSpr->aiState = NULL;
		pXSpr->reference = i;                           pXSpr->busy = pXSpr->state << 16;

		if (!ver7)
			pXSpr->lT |= pXSpr->lB;

		if (!gModernMap) // check if map use modern features by magic sprite
			gModernMap = (pXSpr->rxID == 60 && pXSpr->txID == 60 && pXSpr->command == 100);
	}

	if (loadComments)
	{
		if (!gCmtPrefs.compareCRC || gCommentMgr.CompareCRC(nCRC))
			gCommentMgr.SetCRC(nCRC);
		else
			gCommentMgr.ResetAllTails();
	}

	posx = startposx;
	posy = startposy;
	posz = startposz;
	ang  = startang;

	mapversion = (ver7) ? 7 : 6;
	PropagateMarkerReferences();
	return mapversion;
}

int dbSaveMap(char *filename, BOOL ver7)
{
	char temp[sizeof(pskyoff) + kXSectorDiskSize + kXSpriteDiskSize + kXWallDiskSize];
	int nKey, i, nBytes = 0, nSkySize, nSkyCount;
	short skyBits = pskybits;
	unsigned int nCRC;
	
	BLMMAGIC magic; BLMHEADER_MAIN header; BLMHEADER_EXTRA extra;
	walltype twall; sectortype tsect; spritetype tspr;
	XWALL* pXWall;  XSECTOR* pXSect;  XSPRITE* pXSpr;
	BYTE* pBytes; IOBuffer* pIo;
	
	// save just 1 pskyoff if there is no parallax sectors? (cannot be zero)
	i = numsectors;
	while(i-- && !isSkySector(i, OBJ_FLOOR) && !isSkySector(i, OBJ_CEILING));
	if (i < 0)
		skyBits = 0;
	
	nSkyCount = ClipHigh(1 << skyBits, MAXPSKYTILES);
	nSkySize  = sizeof(int16_t)*nSkyCount;

	memset(temp,    0, sizeof(temp));
	memset(&magic,  0, sizeof(magic));
	memset(&header, 0, sizeof(header));
	memset(&extra,  0, sizeof(extra));
	// -----------------------------------------------------------------------
	nBytes += 4;
	nBytes += sizeof(magic);
	nBytes += sizeof(header);
	nBytes += nSkySize;
	nBytes += (sizeof(sectortype)*numsectors);
	nBytes += (sizeof(walltype)*numwalls);
	if (ver7)
		nBytes += sizeof(extra);
	
	// cannot rely on numxobjects until deal with Cleanup stuff damn it :(((
	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].extra > 0)
			nBytes += kXSectorDiskSize;
	}
	
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra > 0)
			nBytes += kXWallDiskSize;
	}
	
	numsprites = 0;
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus) numsprites++;
		if (sprite[i].extra > 0)
			nBytes += kXSpriteDiskSize;
	}
	
	nBytes += (sizeof(spritetype)*numsprites);
		
	//nBytes += (numsectors*sizeof(sectortype)) + (numxsectors*kXSectorDiskSize);
	//nBytes += (numwalls*sizeof(walltype))     + (numxwalls*kXWallDiskSize);
	//nBytes += (numsprites*sizeof(spritetype)) + (numxsprites*kXSpriteDiskSize);

	// -----------------------------------------------------------------------
	if ((pBytes = (BYTE*)malloc(nBytes)) == NULL)
		return -1;

	pIo = new IOBuffer(nBytes, pBytes);
	// -----------------------------------------------------------------------
	sprintf(magic.sign, kBloodMapSig);
	magic.version       = (ver7) ? ((gCompat.modernMap) ? 0x701 : 0x700) : 0x603;
	// -----------------------------------------------------------------------
	header.startX       = B_LITTLE32(startposx);
	header.startY       = B_LITTLE32(startposy);
	header.startZ       = B_LITTLE32(startposz);
	header.startA       = B_LITTLE16(startang);
	header.startS       = B_LITTLE16(startsectnum);
	header.skyBits      = B_LITTLE16(skyBits);
	header.visibility   = B_LITTLE32(visibility);
	header.mattID       = 0; // no tile 2048 in the top right corner
	header.skyType      = parallaxtype;
	header.revision     = gMapRev;
	header.numsectors   = B_LITTLE16(numsectors);
	header.numwalls     = B_LITTLE16(numwalls);
	header.numsprites   = B_LITTLE16(numsprites);
	// -----------------------------------------------------------------------
	// add some XMAPEDIT specific info
	sprintf(extra.xmpsign, kXMPHeadSig);
	extra.xmpheadver    = kXMPHeadVer2;
	if (gCustomSkyBits)
		extra.xmpMapFlags |= kXmpFlagCustomSkyBits;
	if (gFogMode)
		extra.xmpMapFlags |= kXmpFlagFog;
	
	extra.xmpBoardWidth		= PIX2MET(boardWidth);
	extra.xmpBoardHeight	= PIX2MET(boardHeight);
	extra.xmpPalette		= 0;
	
	// -----------------------------------------------------------------------
	extra.xsprSiz       = kXSpriteDiskSize;
	extra.xwalSiz       = kXWallDiskSize;
	extra.xsecSiz       = kXSectorDiskSize;
	// -----------------------------------------------------------------------
	memcpy(temp, pskyoff, nSkySize);
	// -----------------------------------------------------------------------
	if (ver7)
	{
		dbCrypt((char*)&header, sizeof(header), kMattID2);
		dbCrypt((char*)&extra,  sizeof(extra),  numwalls);
		dbCrypt((char*)temp,    nSkySize,       nSkySize);

		pIo->write(&magic,  sizeof(magic));
		pIo->write(&header, sizeof(header));
		pIo->write(&extra,  sizeof(extra));
		pIo->write(temp,    nSkySize);
	}
	else
	{
		pIo->write(&magic,  sizeof(magic));
		pIo->write(&header, sizeof(header));
		pIo->write(temp,    nSkySize);
	}


	nKey = gMapRev*sizeof(sectortype);
	for (i = 0; i < numsectors; i++)
	{
		tsect = sector[i];
		if (ver7)
			dbCrypt((char*)&tsect, sizeof(sectortype), nKey);

		pIo->write(&tsect, sizeof(sectortype));
		if (sector[i].extra <= 0)
			continue;

		pXSect = &xsector[sector[i].extra];
		BitWriter BW(temp, kXSectorDiskSize);
		BW.write(pXSect->reference,         14);    BW.write(pXSect->state,             1);
		BW.write(pXSect->busy,              17);    BW.write(pXSect->data,              16);
		BW.write(pXSect->txID,              10);    BW.write(pXSect->busyWaveA,         3);
		BW.write(pXSect->busyWaveB,         3);     BW.write(pXSect->rxID,              10);
		BW.write(pXSect->command,           8);     BW.write(pXSect->triggerOn,         1);
		BW.write(pXSect->triggerOff,        1);     BW.write(pXSect->busyTimeA,         12);
		BW.write(pXSect->waitTimeA,         12);    BW.write(pXSect->restState,         1);
		BW.write(pXSect->interruptable,     1);     BW.write(pXSect->amplitude,         8);
		BW.write(pXSect->shadeFreq,         8);     BW.write(pXSect->reTriggerA,        1);
		BW.write(pXSect->reTriggerB,        1);     BW.write(pXSect->shadePhase,        8);
		BW.write(pXSect->shadeWave,         4);     BW.write(pXSect->shadeAlways,       1);
		BW.write(pXSect->shadeFloor,        1);     BW.write(pXSect->shadeCeiling,      1);
		BW.write(pXSect->shadeWalls,        1);     BW.write(pXSect->shade,             8);
		BW.write(pXSect->panAlways,         1);     BW.write(pXSect->panFloor,          1);
		BW.write(pXSect->panCeiling,        1);     BW.write(pXSect->drag,              1);
		BW.write(pXSect->underwater,        1);     BW.write(pXSect->Depth,             3);
		BW.write(pXSect->panVel,            8);     BW.write(pXSect->panAngle,          11);
		BW.write(pXSect->unused1,           1);     BW.write(pXSect->decoupled,         1);
		BW.write(pXSect->triggerOnce,       1);     BW.write(pXSect->isTriggered,       1);
		BW.write(pXSect->key,               3);     BW.write(pXSect->triggerPush,       1);
		BW.write(pXSect->triggerVector,     1);     BW.write(pXSect->triggerReserved,   1);
		BW.write(pXSect->triggerEnter,      1);     BW.write(pXSect->triggerExit,       1);
		BW.write(pXSect->triggerWallPush,   1);     BW.write(pXSect->coloredLights,     1);
		BW.write(pXSect->unused2,           1);     BW.write(pXSect->busyTimeB,         12);
		BW.write(pXSect->waitTimeB,         12);    BW.write(pXSect->stopOn,            1);
		BW.write(pXSect->stopOff,           1);     BW.write(pXSect->ceilpal2,          4);
		BW.write(pXSect->offCeilZ,          32);    BW.write(pXSect->onCeilZ,           32);
		BW.write(pXSect->offFloorZ,         32);    BW.write(pXSect->onFloorZ,          32);
		BW.write(pXSect->marker0,           16);    BW.write(pXSect->marker1,           16);
		BW.write(pXSect->crush,             1);     BW.write(pXSect->ceilXPanFrac,      8);
		BW.write(pXSect->ceilYPanFrac,      8);     BW.write(pXSect->floorXPanFrac,     8);
		BW.write(pXSect->damageType,        3);     BW.write(pXSect->floorpal2,         4);
		BW.write(pXSect->floorYPanFrac,     8);     BW.write(pXSect->locked,            1);
		BW.write(pXSect->windVel,           10);    BW.write(pXSect->windAng,           11);
		BW.write(pXSect->windAlways,        1);     BW.write(pXSect->dudeLockout,       1);
		BW.write(pXSect->bobTheta,          11);    BW.write(pXSect->bobZRange,         5);
		BW.write(pXSect->bobSpeed,          12);    BW.write(pXSect->bobAlways,         1);
		BW.write(pXSect->bobFloor,          1);     BW.write(pXSect->bobCeiling,        1);
		BW.write(pXSect->bobRotate,         1);
		// -----------------------------------------------------------------------
		pIo->write(temp, kXSectorDiskSize);
	}

	nKey = gMapRev*sizeof(sectortype) | kMattID2;
	for (i = 0; i < numwalls; i++)
	{
		twall = wall[i];		
		if (ver7)
			dbCrypt((char*)&twall, sizeof(walltype), nKey);

		pIo->write(&twall, sizeof(walltype));
		if (wall[i].extra <= 0)
			continue;

		pXWall = &xwall[wall[i].extra];
		BitWriter BW(temp, kXWallDiskSize);
		BW.write(pXWall->reference,         14);    BW.write(pXWall->state,             1);
		BW.write(pXWall->busy,              17);    BW.write(pXWall->data,              16);
		BW.write(pXWall->txID,              10);    BW.write(pXWall->unused1,           6);
		BW.write(pXWall->rxID,              10);    BW.write(pXWall->command,           8);
		BW.write(pXWall->triggerOn,         1);     BW.write(pXWall->triggerOff,        1);
		BW.write(pXWall->busyTime,          12);    BW.write(pXWall->waitTime,          12);
		BW.write(pXWall->restState,         1);     BW.write(pXWall->interruptable,     1);
		BW.write(pXWall->panAlways,         1);     BW.write(pXWall->panXVel,           8);
		BW.write(pXWall->panYVel,           8);     BW.write(pXWall->decoupled,         1);
		BW.write(pXWall->triggerOnce,       1);     BW.write(pXWall->isTriggered,       1);
		BW.write(pXWall->key,               3);     BW.write(pXWall->triggerPush,       1);
		BW.write(pXWall->triggerVector,     1);     BW.write(pXWall->triggerTouch,      1);
		BW.write(pXWall->unused2,           2);     BW.write(pXWall->xpanFrac,          8);
		BW.write(pXWall->ypanFrac,          8);     BW.write(pXWall->locked,            1);
		BW.write(pXWall->dudeLockout,       1);     BW.write(pXWall->unused3,           4);
		BW.write(pXWall->unused4,           32);
		// -----------------------------------------------------------------------
		pIo->write(temp, kXWallDiskSize);

	}

	nKey = gMapRev*sizeof(spritetype) | kMattID2;
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum >= kMaxStatus)
			continue;

		tspr = sprite[i];
		if (ver7)
			dbCrypt((char*)&tspr, sizeof(spritetype), nKey);

		pIo->write(&tspr, sizeof(spritetype));
		if (sprite[i].extra <= 0)
			continue;

		pXSpr = &xsprite[sprite[i].extra];
		BitWriter BW(temp, kXSpriteDiskSize);
		BW.write(pXSpr->reference,          14);    BW.write(pXSpr->state,              1);
		BW.write(pXSpr->busy,               17);    BW.write(pXSpr->txID,               10);
		BW.write(pXSpr->rxID,               10);    BW.write(pXSpr->command,            8);
		BW.write(pXSpr->triggerOn,          1);     BW.write(pXSpr->triggerOff,         1);
		BW.write(pXSpr->wave,               2);     BW.write(pXSpr->busyTime,           12);
		BW.write(pXSpr->waitTime,           12);    BW.write(pXSpr->restState,          1);
		BW.write(pXSpr->interruptable,      1);     BW.write(pXSpr->unused1,            2);
		BW.write(pXSpr->respawnPending,     2);     BW.write(pXSpr->unused2,            1);
		BW.write(pXSpr->lT,                 1);     BW.write(pXSpr->dropItem,           8);
		BW.write(pXSpr->decoupled,          1);     BW.write(pXSpr->triggerOnce,        1);
		BW.write(pXSpr->isTriggered,        1);     BW.write(pXSpr->key,                3);
		BW.write(pXSpr->triggerPush,        1);     BW.write(pXSpr->triggerVector,      1);
		BW.write(pXSpr->triggerImpact,      1);     BW.write(pXSpr->triggerPickup,      1);
		BW.write(pXSpr->triggerTouch,       1);     BW.write(pXSpr->triggerSight,       1);
		BW.write(pXSpr->triggerProximity,   1);     BW.write(pXSpr->unused3,            2);
		BW.write(pXSpr->lSkill,             5);     BW.write(pXSpr->lS,                 1);
		BW.write(pXSpr->lB,                 1);     BW.write(pXSpr->lC,                 1);
		BW.write(pXSpr->dudeLockout,        1);     BW.write(pXSpr->data1,              16);
		BW.write(pXSpr->data2,              16);    BW.write(pXSpr->data3,              16);
		BW.write(pXSpr->goalAng,            11);    BW.write(pXSpr->dodgeDir,           2);
		BW.write(pXSpr->locked,             1);     BW.write(pXSpr->medium,             2);
		BW.write(pXSpr->respawn,            2);     BW.write(pXSpr->data4,              16);
		BW.write(pXSpr->unused4,            6);     BW.write(pXSpr->lockMsg,            8);
		BW.write(pXSpr->health,             12);    BW.write(pXSpr->dudeDeaf,           1);
		BW.write(pXSpr->dudeAmbush,         1);     BW.write(pXSpr->dudeGuard,          1);
		BW.write(pXSpr->dudeFlag4,          1);     BW.write(pXSpr->target,             16);
		BW.write(pXSpr->targetX,            32);    BW.write(pXSpr->targetY,            32);
		BW.write(pXSpr->targetZ,            32);    BW.write(pXSpr->burnTime,           16);
		BW.write(pXSpr->burnSource,         16);    BW.write(pXSpr->height,             16);
		BW.write(pXSpr->stateTimer,         16);
		// -----------------------------------------------------------------------
		pIo->write(temp, kXSpriteDiskSize);

	}
	
	if (pIo->tell() == nBytes - 4)
	{
		nCRC = crc32once(pBytes, nBytes - 4);
		pIo->write(&nCRC, 4);

		makeBackup(filename);
		i = (FileSave(filename, pBytes, nBytes)) ? 0 : -1;
		if (i == 0 && gCmtPrefs.enabled) // map saved
		{
			gCommentMgr.SetCRC(nCRC); 		 // update CRC
			gCommentMgr.SaveToIni(filename); // save comments
		}
	}
	else
	{
		i = -2;
	}
	
	free(pBytes);
	return i;
}
