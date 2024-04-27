/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A version of fx.cpp from Nblood adapted for level editor's Preview Mode
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

#ifndef __FX
#define __FX

enum FX_ID {
    FX_NONE = -1,
    FX_0 = 0,
    FX_1,
    FX_2,
    FX_3,
    FX_4,
    FX_5,
    FX_6,
    FX_7,
    FX_8,
    FX_9,
    FX_10,
    FX_11,
    FX_12,
    FX_13,
    FX_14,
    FX_15,
    FX_16,
    FX_17,
    FX_18,
    FX_19,
    FX_20,
    FX_21,
    FX_22,
    FX_23,
    FX_24,
    FX_25,
    FX_26,
    FX_27,
    FX_28,
    FX_29,
    FX_30,
    FX_31,
    FX_32,
    FX_33,
    FX_34,
    FX_35,
    FX_36,
    FX_37,
    FX_38,
    FX_39,
    FX_40,
    FX_41,
    FX_42,
    FX_43,
    FX_44,
    FX_45,
    FX_46,
    FX_47,
    FX_48,
    FX_49,
    FX_50,
    FX_51,
    FX_52,
    FX_53,
    FX_54,
    FX_55,
    FX_56,
	kFXGibThingReplace,
	kFXWaterDripThingReplace,
	kFXBloodDripThingReplace,
    kFXMax
};

struct FXDATA {
    CALLBACK_ID funcID; // callback
    char at1; // detail
    short at2; // seq
    short at4; // flags
    int at6; // gravity
    int ata; // air drag
    int ate;
    short at12; // picnum
    unsigned char at14; // xrepeat
    unsigned char at15; // yrepeat
    short at16; // cstat
    signed char at18; // shade
    char at19; // pal
};

extern FXDATA gFXData[kFXMax];


class CFX {
public:
    void sub_73FB0(int);
    void sub_73FFC(int);
    spritetype * fxSpawn(FX_ID, int, int, int, int, unsigned int time = 0);
    void fxProcess(void);
};

void fxSpawnBlood(spritetype *pSprite, int a2);
void sub_746D4(spritetype *pSprite, int a2);
void fxSpawnEjectingBrass(spritetype *pSprite, int z, int a3, int a4);
void fxSpawnEjectingShell(spritetype *pSprite, int z, int a3, int a4);
extern CFX gFX;

#endif

