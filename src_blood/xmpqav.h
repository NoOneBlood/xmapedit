/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2022: Additional changes by NoOne.
// A lite version of qav.cpp
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
#ifndef __XMPQAV
#define __XMPQAV
#include "common_game.h"

#define kQavOrientationLeft 16384
#define kQavOrientationQ16 32768
#define kQavVersion	0x0200
#define kQavExt ".QAV"


// by NoOne: add sound flags
enum
{
    kFlagSoundKill = 0x01, // mute QAV sounds of same priority
    kFlagSoundKillAll = 0x02, //  mute all QAV sounds

};

struct TILE_FRAME
{
    int picnum;
    int x;
    int y;
    int z;
    int stat;
    signed char shade;
    char palnum;
    unsigned short angle;
};

struct SOUNDINFO
{
    int id;
    unsigned char priority;
    unsigned char flags; // (by NoOne) Various sound flags
    unsigned char range; // (by NoOne) Random sound range
    char reserved[1];
};

struct FRAMEINFO
{
    int nCallbackId;
    SOUNDINFO sound;
    TILE_FRAME tiles[8];
};

struct QAV
{
	char 	sign[4];
	short 	version;
	char	dummy[2];
    int nFrames;
    int ticksPerFrame;
    int duration;
    int x;
    int y;
    int nSprite;
    char pad3[4];
    FRAMEINFO frames[1];
    void Preload(void);
};

void DrawFrame(int x, int y, TILE_FRAME *pTile, int stat, int shade, int palnum);
#endif