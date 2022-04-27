/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to work with xmapedit prefab files of any case.
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

#define kPrefabVersion		3
#define kPrefabFileExt		".pfb"
#define kPrefabIniSection 	"PrefabInfo"
#define kMinPrefabSprites    1
#define kMaxPrefabSprites    512
#define kMaxPrefabNameLength 8



/* extern CHECKBOX_LIST gPrefabType[];

enum {
kPrefabTypeFloor			= 0x0001,
kPrefabTypeCeiling			= 0x0002,
kPrefabTypeWall				= 0x0004,
}; */


int pfbAdd(char *filename, int faceAng);
int pfbInsert(char* file, int sstat, int sect, int x, int y, int z);
int pfbDlgFaceAngDefine();
BOOL pfbRemove(char* filename);



