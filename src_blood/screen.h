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
#ifndef SCREEN_H
#define SCREEN_H
#include "common_game.h"
#include "gfx.h"
#include "gui.h"


extern PALETTE gamepal;
extern RGB *palTable[kPalMax];
extern RGB baseDAC[256];
extern char gFogMode;

void scrInit(void);
void scrLoadPalette(void);
void scrLoadPLUs(void);
void scrSetPalette(int palId);
void scrSetGamma(int nGamma);
void scrSetDac(unsigned char* dapal, unsigned char* dapalgamma);
void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits = 8);

char scrFindClosestColor(int red, int green, int blue);
void scrCreateStdColors(void);
void scrSetMessage(char *__format, ...);
void scrSetLogMessage(char *__format, ...);
void scrDisplayMessage();


void scrSave();
void scrRestore(char freeIt = 1);
#endif