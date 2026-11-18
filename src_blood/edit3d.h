/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
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

#ifndef __EDIT3D_H
#define __EDIT3D_H
#include "hglt.h"

extern int InsertGameObject( int where, int nSector, int x, int y, int z, int nAngle);


void SetSectorShadePhase( int nSector, int shadePhase );
void SetSectorTheta( int nSector, int bobTheta );

int NextSectorNeighborZ(int nSect, int nBaseZ, char topBot, char dir, char hglt = 0);
void ProcessInput3D();
void processMouseLook3D();

void LightBomb(int x, int y, int z, short nSector);
void SetupLightBomb(void);
BOOL F9Menu(void);

char dlgSpriteText();
int helperFillUserDudesList(VOIDLIST* pList);
char helperGetDataForCustomDude(SELITEM* e, int data[3]);
#endif