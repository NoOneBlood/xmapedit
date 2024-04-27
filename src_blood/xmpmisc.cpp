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
#include "xmpmaped.h"
#include "xmparted.h"
#include "xmpevox.h"
#include "seq.h"
#include "xmpmisc.h"
#include "tile.h"
#include "preview.h"
#include "aadjust.h"
#include "mapcmt.h"

int gNextWall[kMaxWalls];

const char* gMapStatsNames[kMapStatMax] =
{
	"Dudes",
	"DudeSpawn",
	"Weapons",
	"Ammo",
	"Powerups",
	"Health",
	"Armor",
	"Inv. Items",
};

NAMED_TYPE gReverseSectorErrors[] =
{
	{ -1,	"Only sliders with non-angled markers and rotate sectors supported" },
	{ -11, 	"No sector found" },
	{-999, NULL},
};


uint16_t MapStats::stats[5][5][kMapStatMax];
int16_t  MapStats::total[5][kMapStatMax];
char MapStats::Inc(XSPRITE* pXSpr, int nWhat)
{
	int i; char exist = 0;
	for (i = 0; i < 5; i++)
	{
		if (pXSpr && (pXSpr->lSkill & (1 << i)) != 0)
			continue;
		
		// when mode is false, it means that sprite actually EXIST!
		if (!pXSpr || !pXSpr->lS) stats[kGameModeSingle][i][nWhat]++,		exist = 1;
		if (!pXSpr || !pXSpr->lC) stats[kGameModeCoop][i][nWhat]++,			exist = 1;
		if (!pXSpr || !pXSpr->lB) stats[kGametModeDeathmatch][i][nWhat]++,	exist = 1;
		if (!pXSpr || !pXSpr->lT) stats[kGameModeFlags][i][nWhat]++,		exist = 1;
		
		if (exist) // total stat
			stats[4][i][nWhat]++;
	}
	
	return exist;
}


void MapStats::IncItemType(int nType, XSPRITE* pXSpr)
{
	if (rngok(nType, kItemWeaponBase, kItemMax))
	{
		switch(nType)
		{
			case kItemHealthDoctorBag:
			case kItemCrystalBall:
			case kItemBeastVision:
			case kItemDivingSuit:
			case kItemJumpBoots:
				Inc(pXSpr, kMapStatInventory);
				break;
			case kItemArmorAsbest:
				Inc(pXSpr, kMapStatPowerup);
				break;
		}
		
		if (rngok(nType, kItemWeaponBase, kItemWeaponMax)) 					Inc(pXSpr, kMapStatWeapons);
		else if (rngok(nType, kItemAmmoBase, kItemAmmoMax)) 				Inc(pXSpr, kMapStatAmmo);
		else if (irngok(nType, kItemHealthDoctorBag, kItemHealthRedPotion))	Inc(pXSpr, kMapStatHealth);
		else if (irngok(nType, kItemFeatherFall, kItemShroomShrink)) 		Inc(pXSpr, kMapStatPowerup);
		else if (irngok(nType, kItemArmorBasic, kItemArmorSuper))			Inc(pXSpr, kMapStatArmor);
	}
}

int MapStats::Get(int nMode, int nWhat)
{
	int nTotal = 0;
	int i = 5;
	int t;
	
	if (total[nMode][nWhat] < 0)
	{
		while(--i >= 0)
		{
			if ((t = stats[nMode][i][nWhat]) > nTotal)
				nTotal = t;
		}
		
		total[nMode][nWhat] = nTotal;
		return nTotal;
	}
	
	return total[nMode][nWhat];
}

int MapStats::Get(int nMode, int nSkill, int nWhat)
{
	return stats[nMode][nSkill][nWhat];
}

void MapStats::Collect()
{
	int i, j;
	
	Clear(0x03);
	i = numsectors;
	while(--i >= 0)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			spritetype* pSpr = &sprite[j];
			XSPRITE* pXSpr = GetXSpr(pSpr);
			
			switch(pSpr->statnum)
			{
				case kStatDude:
					if (pXSpr && Inc(pXSpr, kMapStatDudes))
					{
						if (pXSpr->dropItem)
							IncItemType(pXSpr->dropItem, pXSpr);
					}
					break;
				case kStatThing:
					if (pXSpr && pXSpr->dropItem)
						IncItemType(pXSpr->dropItem, pXSpr);
					break;
				case kStatItem:
					IncItemType(pSpr->type, pXSpr);
					break;
				default:
					switch(pSpr->type)
					{
						case 24:
							if (!gModernMap) break;
							// no break
						case kMarkerDudeSpawn:
							if (pXSpr) Inc(pXSpr, kMapStatSpawn);
							break;
					}
					break;
			}
		}
	}
}

void MapStats::Clear(char which)
{
	if (which & 0x01)
		memset(stats, 0, sizeof(stats));
	
	if (which & 0x02)
	{
		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < kMapStatMax; j++)
				total[i][j] = -1;
		}
	}

}


const char* BIN2TXT::kB2TSign = "NNB2T";
BIN2TXT::BIN2TXT(void)
{
	memset(&bin, 0, sizeof(bin));
	memset(&txt, 0, sizeof(txt));
	memset(&inf, 0, sizeof(inf));
}

int BIN2TXT::EncodeByte(int c)
{
	if (inf.flg & (B2T_ENC_000F|B2T_ENC_MODS))
	{
		if ((inf.flg & B2T_ENC_000F) && irngok(c, 0x00, 0x0F))
			return 'G' + c;
		
		if (inf.flg & B2T_ENC_MODS)
		{
			if ((c % 0x10) == 0)		return 192 + ((c / 0x10) - 1);	// 0x10, 0x20, 0x30 ....
			if ((c % 0x11) == 0)		return 207 + ((c / 0x11) - 1);	// 0x11, 0x22, 0x33 ....
		}
	}
	
	return -1;
}

int BIN2TXT::DecodeByte(int c)
{
	if (inf.flg & (B2T_ENC_000F|B2T_ENC_MODS))
	{
		if ((inf.flg & B2T_ENC_000F) && irngok(c, 'G', 'V'))
			return 0x0F + c - 'V';
		
		if (inf.flg & B2T_ENC_MODS)
		{
			if (irngok(c, 192, 206))	return ((c - 192) + 1) * 0x10;
			if (irngok(c, 207, 221))	return ((c - 207) + 1) * 0x11;
		}
	}
	
	return -1;
}

char BIN2TXT::Encode(void)
{
	const char* sign = kB2TSign;
	int maxLen = strlen(sign)+2+2+8+(inf.len<<3)+(bin.len<<1); // worst case
	int curLen = 0, i = 0, j, n, c;
	uint8_t* pBin;
	char *pTxt;
	
	dassert(txt.ptr == NULL);
	if ((txt.ptr = (char*)malloc(maxLen)) == NULL)
		return 0;
	
	pTxt = txt.ptr;
	
	// header
	//---------------------------------------------------------------------------------
	curLen+=sprintf(&pTxt[curLen], "%s",   sign);				// signature
	curLen+=sprintf(&pTxt[curLen], "%02x", inf.flg);			// flags
	curLen+=sprintf(&pTxt[curLen], "%02x", inf.len);			// extra count
	while(i < inf.len)											// extra data
		curLen+=sprintf(&pTxt[curLen], "%08x", inf.ptr[i++]);
	//---------------------------------------------------------------------------------

	pBin = bin.ptr;
	for (i = 0; i < bin.len; i++)
	{
		c = pBin[i];		
 		if ((n = EncodeByte(c)) >= 0)
		{
			pTxt[curLen++] = n;
		}
		else
		{
			curLen+=sprintf(&pTxt[curLen], "%02x", c);
		}
		
		if (inf.flg & B2T_ENC_BREP)
		{
			j = i, n = 0;
			while(++j < bin.len)
			{
				if (pBin[j] != pBin[i])
					break;
				
				n++;
			}
			
			if (n)
			{
				if (n > 0xFFFF)		curLen+=sprintf(&pTxt[curLen], "g%08x", n);		// giant
				else if (n > 0xFFF)	curLen+=sprintf(&pTxt[curLen], "h%04x", n);		// huge
				else if (n > 0xFF)	curLen+=sprintf(&pTxt[curLen], "l%03x", n);		// large
				else if (n > 0x0F)	curLen+=sprintf(&pTxt[curLen], "m%02x", n);		// medium
				else if (n > 0x01)	curLen+=sprintf(&pTxt[curLen], "s%x", n);		// small
				else if (n == 0x01)	curLen+=sprintf(&pTxt[curLen], "t");			// tiny (repeat 1 time)
				i+=n;
			}
		}
	}
	
	for (i = 0; i < curLen; i++)
	{
		switch(pTxt[i])
		{
			case 'h':	case 'l':
			case 'm':	case 's':
			case 't':	case 'g':
				break;
			default:
				pTxt[i] = toupper(pTxt[i]);
				break;
		}
	}
	
	txt.ptr = (char*)realloc(txt.ptr, curLen);
	txt.len = curLen;
	return 1;
	
}

char BIN2TXT::Decode(void)
{
	IOBuffer io(txt.len, (unsigned char*)txt.ptr);
	const int kBaseAlloc = 0x10000;
	int t = strlen(kB2TSign);
	int i, j, c, r, s = 0;
	char ci[9];
	
	dassert(bin.ptr == NULL);
	
	// header
	// ----------------------------------------------------------------------
	io.read(&ci, t);
	if (memcmp(ci, kB2TSign, t) != 0)
		return 0;
	
	io.read(&ci, 2);	inf.flg = GetDigit(ci, 2);	// flags
	io.read(&ci, 2);	inf.len = GetDigit(ci, 2);	// extra count
	if (inf.len)									// extra data
	{
		if (!inf.ptr)
		{
			inf.ptr = (int32_t*)malloc(inf.len*sizeof(int32_t));
			dassert(inf.ptr != NULL);
		}
		
		for (i = 0; i < inf.len; i++)
			io.read(&ci, 8), inf.ptr[i] = GetDigit(ci, 8);
	}
	// ----------------------------------------------------------------------
	
	j=0;
	while(io.nRemain)
	{
		if (inf.flg & 0x07)
		{
			io.read(&t, 1);

			switch( t )
			{
				case 't':	r = 1;									break;	// tiny
				case 's':	io.read(&ci, 1), r = GetDigit(ci, 1);	break;	// small
				case 'm':	io.read(&ci, 2), r = GetDigit(ci, 2);	break;	// medium
				case 'l':	io.read(&ci, 3), r = GetDigit(ci, 3);	break;	// large
				case 'h':	io.read(&ci, 4), r = GetDigit(ci, 4);	break;	// huge
				case 'g':	io.read(&ci, 8), r = GetDigit(ci, 8);	break;	// giant
				default:
					if (j < 2)
					{
						if ((c = DecodeByte(t)) >= 0)
						{
							r = 1, j = 0;
							break;
						}
						else
						{
							ci[j++] = t;
							r = 0;
						}
					}
					
					if (j == 2)
					{
						c = GetDigit(ci, 2);
						r = 1, j = 0;
					}
					break;
			}
		}
		else
		{
			io.read(&ci, 2), c = GetDigit(ci, 2);
			r = 1;
		}
		
		if (r)
		{
			if (bin.len + r >= s)
			{
				s = ClipLow(s, bin.len + r) + kBaseAlloc;
				bin.ptr = (uint8_t*)realloc(bin.ptr, s);
				dassert(bin.ptr != NULL);
			}
			
			memset(&bin.ptr[bin.len], c, r);
			bin.len+=r;
		}
	}
	
	return 1;
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
	
	sprintf(buff, "BUILD [%s]", build_date);
	gfxPrinTextShadow(18, y, gStdColor[18], strupr(buff), pFont);
	
	gfxSetColor(gStdColor[kColorRed]); y = ydim - 27;
	gfxHLine(y, 18, xdim - 18); y++;
	gfxHLine(y, 18, xdim - 18); y++;
	y+=(pFont->height >> 1);
	
	
	sprintf(buff, "www.cruo.bloodgame.ru/xmapedit"); i = gfxGetTextLen(strupr(buff), pFont);
	gfxPrinTextShadow(xdim - i - 16, y, gStdColor[18], buff, pFont);
	showframe();
	
}

void Delay(int time) {
	int done = totalclock + time;
	while (totalclock < done);
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
		if (gCommentMgr.IsBind(OBJ_WALL, oxindex) >= 0)
			return FALSE;
		
		XWALL pXModel; memset(&pXModel, 0, sizeof(XWALL));
		pXModel.reference = xwall[oxindex].reference;
		return (memcmp(&pXModel, &xwall[oxindex], sizeof(XWALL)) == 0);
	}
	else if (otype == OBJ_SPRITE)
	{
		XSPRITE pXModel; memset(&pXModel, 0, sizeof(XSPRITE));
		pXModel.reference = xsprite[oxindex].reference;
		
		return (memcmp(&pXModel, &xsprite[oxindex], sizeof(XSPRITE)) == 0);
	}
	else if (otype == OBJ_SECTOR || otype == OBJ_CEILING || otype == OBJ_FLOOR)
	{
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

int getDataOf(int nData, int nType, int nID)
{
	switch (nType)
	{
		case OBJ_SPRITE:
			switch (nData)
			{
				case 0:  return xsprite[sprite[nID].extra].data1;
				case 1:  return xsprite[sprite[nID].extra].data2;
				case 2:  return xsprite[sprite[nID].extra].data3;
				case 3:  return xsprite[sprite[nID].extra].data4;
			}
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			return xwall[wall[nID].extra].data;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			return xsector[sector[nID].extra].data;
	}
	
	return -1;
	
}

char setDataOf(int nData, int nVal, int nType, int nID)
{
	switch (nType)
	{
		case OBJ_SPRITE:
			switch (nData)
			{
				case 0:	xsprite[sprite[nID].extra].data1 = nVal; break;
				case 1:	xsprite[sprite[nID].extra].data2 = nVal; break;
				case 2:	xsprite[sprite[nID].extra].data3 = nVal; break;
				case 3:	xsprite[sprite[nID].extra].data4 = nVal; break;
			}
			return 1;
		case OBJ_WALL:
		case OBJ_MASKED:
			xwall[wall[nID].extra].data = nVal;
			return 1;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			xsector[sector[nID].extra].data = nVal;
			return 1;
	}
	
	return 0;
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


short name2TypeId(char* defName)
{
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
		for (i = 0; i < kMaxObjectType; i++)
		{
			if (gSpriteNames[i])
				sprintf(names[0], gSpriteNames[i]);
			
			if (gSpriteCaptions[i])
				sprintf(names[1], gSpriteCaptions[i]);
			
			//if (spriteNames[i].name)    sprintf(names[0], spriteNames[i].name);
			//if (spriteNames[i].caption) sprintf(names[1], spriteNames[i].caption);
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
						return i;
				}
			}
		}

		Alert("Type not found for \"%s\"", defName);
		nType = name2TypeId(defName);
	}
	else // all symbols are digits, just search by typeId
	{ 
		nType = (short) ClipRange(atoi(typeName), 0, kMaxObjectType - 1);
		if (gSpriteNames[nType])
			return nType;

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

int getHighlightedObject() {
	
	if (qsetmode == 200)
	{
		sectorhighlight = searchsector;
		pointhighlight = -1;
		linehighlight = -1;
		
		switch (searchstat)
		{
			case OBJ_SPRITE:
				pointhighlight = searchwall + 16384;
				return 200;
			case OBJ_WALL:
			case OBJ_MASKED:
				pointhighlight = searchwall;
				linehighlight = searchwall;
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

void hit2pos(int* rtx, int* rty, int* rtz, int32_t clipmask) {
	
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	int hitx = 0, hity = 0, hitz = 0, x = 0, y = 0, z = 0;
	short sect = -1;
	
	if (qsetmode == 200)
	{
		camHitscan(&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, clipmask);
		*rtx = hitx; *rty = hity;
		if (rtz)
			*rtz = hitz;
	}
	else
	{
		int x2d, y2d;
		gScreen2D.GetPoint(searchx,searchy,&x2d,&y2d);
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
		camHitscan(&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, clipmask);
	}
	else
	{
		int x2d, y2d;
		gScreen2D.GetPoint(searchx,searchy,&x2d,&y2d);
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



BOOL isModernMap() {

	if (gCompat.modernMap || gModernMap) return TRUE;
	if (numsprites >= kVanillaMaxSprites || numxsprites >= kVanillaMaxXSprites) return TRUE;
	if (numsectors >= kVanillaMaxSectors || numxsectors >= kVanillaMaxXSectors) return TRUE;
	if (numwalls   >= kVanillaMaxWalls   || numxwalls   >= kVanillaMaxXWalls)   return TRUE;	
	return FALSE;
	
}

BOOL isMultiTx(short nSpr) {
	
	int j = 0;
	if (sprite[nSpr].statnum < kStatFree && (sprite[nSpr].type == 25 || sprite[nSpr].type == 26))
	{
		for (BYTE i = 0; i < 4; i++)
			if (chlRangeIsFine(getDataOf(i, OBJ_SPRITE, nSpr))) j++;
	}
	
	return (j > 0);
}

BOOL multiTxGetRange(int nSpr, int out[4])
{
	int j;
	XSPRITE* pXSpr = &xsprite[sprite[nSpr].extra];
	memset(out, 0, sizeof(int)*4);
	
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
	
	int j; int txrng[4];

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

void toggleResolution(int fs, int xres, int yres, int bpp)
{
	int mode = qsetmode;
	scrSetGameMode(fs, xres, yres, bpp);

	if (ED23)				mode = 0x02|0x80;
	else if (mode == 200)	mode = 0x01|0x80;
	else					mode = 0x00|0x80;
	
	if (tileLoadTile(gSysTiles.drawBuf))
	{
		tileFreeTile(gSysTiles.drawBuf);
		tileAllocTile(gSysTiles.drawBuf, xdim, ydim);
	}
	
	gTileView.InitWindow();
	
	flushperms();
	xmpSetEditMode(mode);	
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
	
	int secrets = 0, nSprite = -1;
	int i, j, s, e;
	
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

short getSector() {
	
	int sect = -1; int x, y, z; // dummy
	hit2sector(&sect, &x, &y, &z, 0, 0);
	return (short)sect;
	
}

void TranslateWallToSector( void )
{
	switch (searchstat)
	{
		case OBJ_WALL:
			if ((searchsector = wall[searchwall2].nextsector) >= 0)
			{
				searchindex = searchsector;
				break;
			}
			// no break
		case OBJ_MASKED:
			searchsector = sectorofwall(searchwall);
			searchindex = searchsector;
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

int camHitscan(short* hsc, short* hwl, short* hsp, int* hx, int* hy, int* hz, unsigned int clipMask)
{
	int x1 = windowx1;	int x2 = windowx2;
	int y1 = windowy1;	int y2 = windowy2;
	int wh = x2-x1;
	int hg = y2-y1;
	
	int dx, dy, dz;
    dx = divscale30(1, viewingrange);
	dy = divscale14(searchx-(wh>>1), wh>>1);
	dz = divscale27(searchy-(hg>>1), scale(wh,yxaspect,320)) + ((100-horiz)*2000);
	RotateVector(&dx, &dy, ang);
	
	*hsc = *hwl = *hsp = -1;
	return hitscan(posx, posy, posz, cursectnum, dx, dy,
			dz, hsc, hwl, hsp, hx, hy, hz, clipMask);
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

char dlgSaveAndOrQuit(void)
{
	int r = 1;
	char artChanges = (gArtEd.asksave != 0);
	char mapChanges = (gMapLoaded && ((numsectors > 0 && numwalls > 0) || numsprites > 0));
	char askSave = (artChanges || mapChanges);
	
	if (askSave)
	{
		switch(r = DlgSaveChanges("Save changes?", gArtEd.asksave))
		{
			case 2:
			case 1:
				if (mapChanges)
					boardSave(gPaths.maps, FALSE);
				
				if (r > 1)
				{
					extVoxUninit();
					tileUninitSystemTiles();
					artedSaveArtFiles();
					artedSaveDatFiles();
				}
				// no break
			case 0:
				xmpQuit();
				return 1;
			default:
				return 0;
		}
	}
	
	if (Confirm("Really want to quit?"))
	{
		xmpQuit();
		return 1;
	}
	
	return 0;
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

char hasByte(BYTE* pBytes, int l, char nByte)
{
	while(--l >= 0)
	{
		if (pBytes[l] == nByte)
			return 1;
	}
	
	return 0;
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

/*int getChannelOf(int nType, int nID, char tx)
{
	int nRetn = -1;
	switch(nType)
	{
		case OBJ_SPRITE:
			if (sprite[nID].extra > 0)
				nRetn = (tx) ? xsprite[sprite[nID].extra].txID : xsprite[sprite[nID].extra].rxID;
			
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			if (wall[nID].extra > 0)
				nRetn = (tx) ? xwall[wall[nID].extra].txID : xwall[wall[nID].extra].rxID;
			
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			if (sector[nID].extra > 0)
				nRetn = (tx) ? xsector[sector[nID].extra].txID : xsector[sector[nID].extra].rxID;
			
			break;
	}
	
	return nRetn;
}*/


void collectUsedChannels(unsigned char used[1024])
{
	memset(used, 0, 1024);
	int i, s, e;
	int tx[4];
	
	i = numsectors;
	while(--i >= 0)
	{
		XSECTOR* pXObj = GetXSect(&sector[i]);
		if (pXObj)
		{
			used[pXObj->txID] = 1;
			used[pXObj->rxID] = 1;
		}
		
		getSectorWalls(i, &s, &e);
		while(s <= e)
		{
			XWALL* pXObj = GetXWall(&wall[s++]);
			if (pXObj)
			{
				used[pXObj->txID] = 1;
				used[pXObj->rxID] = 1;
			}
		}
		
		for (s = headspritesect[i]; s >= 0; s = nextspritesect[s])
		{
			XSPRITE* pXObj = GetXSpr(&sprite[s]);
			if (pXObj)
			{
				used[pXObj->txID] = 1;
				used[pXObj->rxID] = 1;
				if (isMultiTx(s))
				{
					if (multiTxGetRange(s, tx))
					{
						for (e = tx[0]; e <= tx[1]; e++) used[e] = 1;
					}
					else
					{
						for (e = 0; e < 4; e++) used[tx[e]] = 1;
					}
				}
			}
		}
	}
}

int collectObjectsByChannel(int nChannel, BOOL rx, OBJECT_LIST* pOut, char flags)
{
	BOOL ok, link = FALSE, unlink = FALSE;
	int c = 0, j, s, e, f;
	int i = numsectors;
		
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

int getShadeOf(int oType, int oIdx)
{
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

void setShadeOf(int nShade, int oType, int oIdx)
{
	switch (oType)
	{
		case OBJ_WALL:
		case OBJ_MASKED:
			wall[oIdx].shade = (signed char)ClipRange(nShade, -128, 63);
			break;
		case OBJ_FLOOR:
			sector[oIdx].floorshade = (signed char)ClipRange(nShade, -128, 63);
			break;
		case OBJ_CEILING:
			sector[oIdx].ceilingshade = (signed char)ClipRange(nShade, -128, 63);
			break;
		case OBJ_SPRITE:
			sprite[oIdx].shade = (signed char)ClipRange(nShade, -128, 63);
			break;
	}
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
	{
		int nCnt = arg.pluCnt - 2;
		if (nCnt > 3)
			qsort(&arg.pluInfo[2], arg.pluCnt-2, sizeof(arg.pluInfo[0]), (int(*)(const void*,const void*))cmpPluEfficency);
	}
	
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






int isMarkerSprite(int nSpr)
{
	spritetype* pSpr = &sprite[nSpr];
	switch(pSpr->type)
	{
		case kMarkerAxis:
		case kMarkerOn:
		case kMarkerOff:
			if (pSpr->statnum != kStatMarker || !rngok(pSpr->owner, 0, numsectors)) break;
			// no break
		case kMarkerPath:
			return pSpr->type;
	}
	
	return 0;
}


int reverseSectorPosition(int nSect, char flags)
{
	sectortype* pSect = &sector[nSect];
	if (!rngok(pSect->extra, 1, kMaxXSectors))
		return -1;
	
	XSECTOR* pXSect = &xsector[pSect->extra];
	spritetype* pMark0, *pMark1 = NULL;
	
	if (pXSect->marker0 < 0)
		return -1;
	
	if (pSect->type == kSectorRotateStep)
		flags &= ~0x03;
	

	switch(pSect->type)
	{
		case kSectorSlideMarked:
		case kSectorSlide:
			if (pXSect->marker1 < 0) return -1;
			pMark1 = &sprite[pXSect->marker1];
			// no break
		case kSectorRotate:
		case kSectorRotateMarked:
		case kSectorRotateStep:
			pMark0 = &sprite[pXSect->marker0];
			
			// !!!
			// don't know why sliders can't return to the rest
			// state once rotated, so for now end with an error
			if (pMark1 && (pMark0->ang || pMark1->ang))
				return -1;
			
			trBasePointInit();
			if (pMark1) TranslateSector(nSect, 0, -65536, pMark0->x, pMark0->y, pMark0->x, pMark0->y, pMark0->ang, pMark1->x, pMark1->y, pMark1->ang, pSect->type == kSectorSlide);
			else TranslateSector(nSect, 0, -65536, pMark0->x, pMark0->y, pMark0->x, pMark0->y, 0, pMark0->x, pMark0->y, pMark0->ang, pSect->type == kSectorRotate);
			
			setBaseWallSect(nSect);
			setBaseSpriteSect(nSect);
			
			if (pMark1)
			{
				//TranslateSector(nSect, 0, 65536, pMark0->x, pMark0->y, pMark0->x, pMark0->y, pMark0->ang, pMark1->x, pMark1->y, pMark1->ang, pSect->type == kSectorSlide);
				if (flags & 0x02)
				{
					swapValues(&pMark1->x, &pMark0->x);
					swapValues(&pMark1->y, &pMark0->y);
				}
				
				//int ta = pMark1->ang;
				//pMark1->ang = pMark0->ang;
				//pMark0->ang = ta;
				
				//pMark1->ang = revertValue(pMark1->ang);
				//pMark0->ang = revertValue(pMark0->ang);
			}
			else if (flags & 0x02)
			{
				pMark0->ang = revertValue(pMark0->ang);
			}
			
			if (flags & 0x01)
			{
				pXSect->state = !pXSect->state;
			}
			break;
		default:
			return -1;
	}
	
	return 1;
}

int countSpritesOfSector(int nSect)
{
	int c = 0;
	for (int i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
		c++;
	
	return c;
}




char isIslandSector(int nSect)
{
	int nParent, s, e;
	getSectorWalls(nSect, &s, &e);
	
	nParent = wall[s].nextsector;
	while(s <= e && wall[s++].nextsector == nParent);
	return (s > e);
}

char isNextWallOf(int nSrc, int nDest)
{
	int x1, y1, x2, y2, x3, y3, x4, y4;
	getWallCoords(nSrc, &x1, &y1, &x2, &y2);
	getWallCoords(nDest, &x3, &y3, &x4, &y4);
	
	return (x3 == x2 && y3 == y2 && x4 == x1 && y4 == y1);
}

int findNextWall(int nWall)
{
	int i;
	if (wall[nWall].nextwall >= 0)
		return wall[nWall].nextwall;
	
	i = numwalls;
	while(--i >= 0 && !isNextWallOf(nWall, i));
	return i;
}

void posChg(int* x, int* y, int bx, int by, char chgRel)
{
	if (chgRel) *x += bx, *y += by;
	else *x = bx, *y = by;
}

void posFlip(int* x, int* y, int cx, int cy, char flipX)
{
	if (flipX)	*x = (cx - *x) + cx;
	else		*y	= (cy - *y) + cy;
}

void posRotate(int* x, int* y, int rAng, int cx, int cy, char rPoint)
{
	int dx = *x, dy = *y;
	
	if (rPoint)			RotatePoint(&dx, &dy, rAng, cx, cy);
	else if (rAng > 0)	dx = cx + cy - *y, dy = cy + *x - cx; // Ken's four side rotation
	else if (rAng < 0)	dx = cx + *y - cy, dy = cy + cx - *x; // Ken's four side rotation
	*x = dx, *y = dy;
}


char scanWallOfSector(SCANWALL* pIn, SCANWALL* pOut)
{
	sectortype* pSect = &sector[pIn->s];
	short* pWallCstat = (short*)malloc(pSect->wallnum*(sizeof(short)+1));
	short* pSprCstat = NULL;
	int nSect = pIn->s, i, j, s, e;
	int hx, hy, hz;
	short hsp;

	
	int backSlope[2]	= {pSect->floorslope, pSect->ceilingslope};
	int backGoal[2]		= {hitscangoalx, hitscangoaly};

	pOut->w = -1;
	if (pWallCstat)
	{
		getSectorWalls(nSect, &s, &e);
		for (i = s, j = 0; i <= e; i++)
		{
			pWallCstat[j++] = wall[i].cstat;
			wall[i].cstat = kWallBlock;
		}
	}
	
	if ((i = countSpritesOfSector(nSect)) > 0)
	{
		if ((pSprCstat = (short*)malloc(i*sizeof(short))) != NULL)
		{
			for (i = headspritesect[nSect], j = 0; i >= 0; i = nextspritesect[i])
			{
				pSprCstat[j++] = sprite[i].cstat;
				sprite[i].cstat = kSprFloor;
			}
		}
	}
	
	pSect->floorslope = pSect->ceilingslope = 0;
	hitscangoalx = hitscangoaly = 0x1fffffff;
	
	hitscan
	(
		pIn->pos.x, pIn->pos.y, pIn->pos.z, nSect,
		Cos(pIn->pos.a)>>16, Sin(pIn->pos.a)>>16, 0,
		&pOut->s, &pOut->w, &hsp,
		&hx, &hy, &hz,
		BLOCK_MOVE
	);
	
	hitscangoalx = backGoal[0];
	hitscangoaly = backGoal[1];
	
	pSect->floorslope	= backSlope[0];
	pSect->ceilingslope = backSlope[1];
	
	if (pWallCstat)
	{
		for (i = s, j = 0; i <= e; i++)
			wall[i].cstat = pWallCstat[j++];
		
		free(pWallCstat);
	}
	
	if (pSprCstat)
	{
		for (i = headspritesect[nSect], j = 0; i >= 0; i = nextspritesect[i])
			sprite[i].cstat = pSprCstat[j++];
		
		free(pSprCstat);
	}
	
	if (pOut->w >= 0)
	{
		pOut->pos.New((GetWallAngle(pOut->w) + kAng90) & kAngMask, hx, hy, hz);
		return 1;
	}
	else
	{
		pOut->pos.New(pIn->pos.a, hx, hy, hz);
	}
	
	return 0;
}

BOOL testXSectorForLighting(int nXSect)
{
	int sum = 0;
	XSECTOR* sc = &xsector[nXSect];
	if (obsoleteXObject(OBJ_FLOOR, nXSect))
		return FALSE;

	sum  = sc->state + sc->txID + sc->rxID + sc->command + sc->triggerOn + sc->triggerOff;
	sum += sc->decoupled + sc->triggerOnce + sc->locked + sc->interruptable + sc->dudeLockout;
	sum += sc->triggerOn + sc->busyTimeA + sc->busyWaveA + sc->reTriggerA;
	sum += sc->waitTimeA + sc->triggerOff + sc->busyTimeB + sc->busyWaveB + sc->reTriggerB;
	sum += sc->waitTimeB + sc->triggerPush + sc->triggerWallPush + sc->triggerEnter + sc->triggerExit;
	sum += sc->key + sc->Depth + sc->underwater + sc->crush + sc->data;
	return (BOOL)(sum == 0);
}

int roundAngle(int nAng, int nAngles)
{
	int i, a = 0;
	int16_t *angles = new int16_t [nAngles];
	int n = 2048 / nAngles;
	
	for (i = 0; i < nAngles; i++)
	{
		angles[i] = a;
		a+=n;
	}

	while(--i >= 0 && !rngok(nAng, angles[i], angles[i]+n))
	delete [] angles;
	return angles[i];
}


XSPRITE* GetXSpr(spritetype* pSpr)
{
	return (pSpr->extra > 0) ? &xsprite[pSpr->extra] : NULL;
}

XSECTOR* GetXSect(sectortype* pSect)
{
	return (pSect->extra > 0) ? &xsector[pSect->extra] : NULL;
}

XWALL* GetXWall(walltype* pWall)
{
	return (pWall->extra > 0) ? &xwall[pWall->extra] : NULL;
}

int findNamedID(const char* str, NAMED_TYPE* pDb, int nLen)
{
	while(--nLen >= 0)
	{
		if (pDb[nLen].name && Bstrcasecmp(str, pDb[nLen].name) == 0)
			return pDb[nLen].id;
	}

    return -1;
}

int words2flags(const char* str, NAMED_TYPE* pDb, int nLen)
{
    char tmp[256];
    int nRetn = 0, nPar, i;
    
	i = 0;
    while (i < nLen && enumStr(i++, str, tmp))
    {
        if ((nPar = findNamedID(tmp, pDb, nLen)) >= 0)
            nRetn |= nPar;
    }

    return nRetn;
}



