/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
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
#pragma once

#pragma pack(push, 1)
struct RGB {
    unsigned char r, g, b;
};

#pragma pack(pop)

typedef RGB PALETTE[256];
extern PALETTE gamepal;
extern char message[256];
extern int messageTime;
extern RGB *palTable[5];

extern bool DacInvalid;
extern RGB curDAC[256];
extern RGB baseDAC[256];
extern int gGammaLevels;
extern char gFogMode;
extern int32_t gBrightness;
void scrCreateStdColors(void);
void scrResetPalette(void);
void gSetDacRange(int start, int end, RGB *pPal);
void scrLoadPLUs(void);
void scrLoadPalette(void);
void scrSetMessage(char *__format, ...);
void scrDisplayMessage(int a1);
void scrSetPalette(int palId, bool updDac = true);
void scrSetGamma(int nGamma);
void scrSetupFade(char red, char green, char blue);
void scrSetupUnfade(void);
void scrFadeAmount(int amount);
void scrSetDac(void);
void scrSetDac2(unsigned char* dapal, unsigned char* dapalgamma);
void scrInit(void);
void scrUnInit(bool engineUninit = true);
void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits = 8);
char scrFindClosestColor(int red, int green, int blue);
void scrNextPage(void);