/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
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

struct AUTODATA {	
	int     group;
	BOOL    exception;  // do not auto-set some of properties
	BOOL    xsprite;	// requires xsprite?
	short	type;
	short   seq;		// use seq info instead of info from fields below
	short	picnum;
	short	xrepeat, yrepeat;
	schar	hitBit;
	short	plu;
};

struct SYS_STATNUM_GROUP {
	
	unsigned int check				: 3;
	unsigned int statnum 			: 11;
	unsigned int typeMin 			: 11; // inclusive!
	unsigned int typeMax 			: 11; // inclusive!
	
	unsigned short* enumArray;
	const unsigned int enumLen		: 11;
};

extern AUTODATA autoData[];
extern SYS_STATNUM_GROUP sysStatData[];

extern const short autoDataLength;

void AutoAdjustSprites(void);
void adjSetApperance(spritetype* pSprite, int idx);
BOOL adjSpriteByType(spritetype* pSprite);
int adjCountSkips(int pic);
int adjIdxByTileInfo(int pic, int skip);
int adjFillTilesArray(int objectGroup);
int adjIdxByType(int nType);

BOOL sysStatReserved(int nStat);
BOOL sysStatCanChange(spritetype* pSpr);

void CleanUp();
void CleanUpStatnum(int nSpr);
void CleanUpMisc();