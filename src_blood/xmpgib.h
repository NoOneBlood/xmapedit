/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A version of gib.cpp from Nblood adapted for level editor's Preview Mode
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

#ifndef __XMPGIB
#define __XMPGIB

struct GIBFX
{
    FX_ID at0;
    int at1;
    int chance;
    int at9;
    int atd;
    int at11;
};


struct GIBTHING
{
    short at0;
    short at4;
    int chance;
    int atc;
    int at10;
};

struct GIBLIST
{
    GIBFX *at0;
    int at4;
    GIBTHING *at8;
    int atc;
    int at10;
};

enum GIBTYPE {
    GIBTYPE_0 = 0,
    GIBTYPE_1,
    GIBTYPE_2,
    GIBTYPE_3,
    GIBTYPE_4,
    GIBTYPE_5,
    GIBTYPE_6,
    GIBTYPE_7,
    GIBTYPE_8,
    GIBTYPE_9,
    GIBTYPE_10,
    GIBTYPE_11,
    GIBTYPE_12,
    GIBTYPE_13,
    GIBTYPE_14,
    GIBTYPE_15,
    GIBTYPE_16,
    GIBTYPE_17,
    GIBTYPE_18,
    GIBTYPE_19,
    GIBTYPE_20,
    GIBTYPE_21,
    GIBTYPE_22,
    GIBTYPE_23,
    GIBTYPE_24,
    GIBTYPE_25,
    GIBTYPE_26,
    GIBTYPE_27,
    GIBTYPE_28,
    GIBTYPE_29,
    GIBTYPE_30,
    kGibMax
};

extern GIBLIST gibList[kGibMax];

class CGibPosition {
public:
    int x, y, z;
    CGibPosition(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};

class CGibVelocity {
public:
    int vx, vy, vz;
    CGibVelocity(int _vx, int _vy, int _vz) : vx(_vx), vy(_vy), vz(_vz) {}
};

void GibSprite(spritetype *pSprite, GIBTYPE nGibType, CGibPosition *pPos, CGibVelocity *pVel);
void GibWall(int nWall, GIBTYPE nGibType, CGibVelocity *pVel);
void gibPrecache(void);

#endif