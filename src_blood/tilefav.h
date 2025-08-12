/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Initialization & configuration of xmapedit.
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
#ifndef __TILEFAV_H
#define __TILEFAV_H
#define kMaxFavoriteTiles 1024
#define kFavTilesFileName "xmapedit/xmapedit.ftl"

struct GAME_OBJECT_TILE {
    short pic;
    short type;
};
extern GAME_OBJECT_TILE gFavTiles[kMaxFavoriteTiles];
extern short gFavTilesC;

void favoriteTileInit();
int favoriteTileAddSimple(short type, short picnum);
BOOL favoriteTileRemove(int nTile);
int favoriteTileSelect(int startPic, int nDefault, BOOL returnTile, int objType);
spritetype* favTileInsert(int where, int nSector, int x, int y, int z, int nAngle);
int tileInFaves(int picnum);
void favoriteTileSave();
#endif