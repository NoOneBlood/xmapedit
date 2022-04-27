/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of actor.cpp from Nblood adapted for Preview Mode
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
#ifndef __XMPACT
#define __XMPACT

#include "build.h"
#include "common_game.h"

struct POSTPONE {
    short at0;
    short at2;
};


struct EXPLODE_INFO
{
    BYTE repeat;
    char dmg;
    char dmgRng;
    int radius;
    int dmgType;
    int burnTime;
    int ticks;
    int quakeEffect;
    int flashEffect;
};

struct MissileType
{
    short picnum;
    int velocity;
    int angleOfs;
    unsigned char xrepeat;
    unsigned char yrepeat;
    char shade;
    unsigned char clipDist;
};

extern EXPLODE_INFO explodeInfo[kExplosionMax];
extern MissileType missileInfo[18];
extern POSTPONE gPost[];
extern int gPostCount;

inline BOOL IsThingSprite( spritetype *pSprite )
{
	return (pSprite->type >= kThingBase && pSprite->type < kThingMax) ? TRUE : FALSE;
}

inline BOOL IsDudeSprite( spritetype *pSprite )
{
	return (pSprite->type >= kDudeBase && pSprite->type < kDudeMax) ? TRUE : FALSE;
}

inline BOOL IsPlayerSprite( spritetype *pSprite )
{
	return (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8) ? TRUE : FALSE;
}

void actProcessSprites();
void actBuildMissile(spritetype* pMissile, int nXSprite, int nSprite);
spritetype* actFireMissile(spritetype *pSprite, int a2, int a3, int a4, int a5, int a6, int nType);
void actAirDrag(spritetype *pSpr, int nDrag);
void actAirDrag(int* vx, int* vy, int* vz, int nSect, int nDrag);
void actExplodeSprite(spritetype *pSprite);
spritetype *actSpawnSprite(int nSector, int x, int y, int z, int nStatus, BOOL bAddXSprite);
spritetype *actSpawnSprite( spritetype *pSource, int nStat );
spritetype *actDropObject(spritetype *pActor, int nObject);
spritetype *actSpawnDude(spritetype *pSource, short nType, int a3, int a4);
void actActivateGibObject(spritetype *pSprite, XSPRITE *pXSprite);
void actPostSprite(int nSprite, int nStatus);
void actPostProcess(void);
BOOL actCheckProximity(int x, int y, int z, int nSector, int dist);
#endif