/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of game's view.cpp adapted for xmapedit.
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
#ifndef __XMPVIEW
#define __XMPVIEW
enum VIEW_EFFECT {
kViewEffectTorchHigh            =  4,
kViewEffectTorchLow             =  5,
kViewEffectSmokeHigh            =  6,
kViewEffectSmokeLow             =  7,
kViewEffectCandleHalo           = 12,
kViewEffectHighlight                ,
kViewEffectAngle                    ,
kViewEffectMiniDude                 ,
kViewEffectMiniCustomDude           ,
kViewEffectGameMode                 ,
kViewEffectCdudeVersion             ,
kViewEffectMax                      ,
};

void viewDoQuake(int strength, int* x, int* y, int* z, short* a, int* h);
void viewProcessSprites(int x, int y, int z, int a);
int viewSpriteShade(int nShade, int nTile, int nSect);
void viewWallHighlight(int nWall, int nSect, char how = 0x0, BOOL testPoint = TRUE);
spritetype *viewInsertTSprite( int nSector, int nStatus, spritetype *pSource = NULL );
#endif