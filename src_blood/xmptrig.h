/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of triggers.cpp from Nblood adapted for level editor's Preview Mode
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

#ifndef __XMPTRIG
#define __XMPTRIG
#include "common_game.h"
#include "db.h"
#include "eventq.h"

enum BUSYID {
    BUSYID_0 = 0,
    BUSYID_1,
    BUSYID_2,
    BUSYID_3,
    BUSYID_4,
    BUSYID_5,
    BUSYID_6,
    BUSYID_7,
};

#define kMaxBusyCount 128
struct BUSY {
    int at0;
    int at4;
    int at8;
    BUSYID atc;
};

extern BUSY gBusy[kMaxBusyCount];
extern int gBusyCount;

void AlignSlopes(void);
void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int command);
void trMessageSector(unsigned int nSector, EVENT event);
void trTriggerWall(unsigned int nWall, XWALL *pXWall, int command);
void trMessageWall(unsigned int nWall, EVENT event);
void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int command);
void trMessageSprite(unsigned int nSprite, EVENT event);
void trProcessBusy(void);
void trInit(void);
void trTextOver(int nId);
char SetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
char SetWallState(int nWall, XWALL* pXWall, int nState);
char SetSectorState(int nSector, XSECTOR* pXSector, int nState);
int TeleFrag(int nKiller, int nSector);
void SectorStartSound(int nSector, int nState);
void SectorEndSound(int nSector, int nState);
void ActivateGenerator(int nSprite);
void trBasePointInit();
void TranslateSector(int nSector, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12);
void ZTranslateSector(int nSector, XSECTOR *pXSector, int a3, int a4);

void setBaseWallSect(int nSect);
void setBaseSpriteSect(int nSect);
#endif