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

extern int InsertGameObject( int where, int nSector, int x, int y, int z, int nAngle);

void SetWave( int nSector, int nWave );
void SetSectorShadePhase( int nSector, int shadePhase );
void SetSectorTheta( int nSector, int bobTheta );

void SetFloorZ( int nSector, int z );
void SetCeilingZ( int nSector, int z );
void LowerFloor( int nSector, int nStep );
void LowerCeiling( int nSector, int nStep );
void RaiseFloor( int nSector, int nStep );
void RaiseCeiling( int nSector, int nStep );
void SetFloorRelative( int nSector, int dz );
void SetCeilingRelative( int nSector, int dz );
void SetFloorSlope( int nSector, int nSlope );
void SetCeilingSlope( int nSector, int nSlope );

void LowerSprite( spritetype *pSprite, int nStep );
void RaiseSprite( spritetype *pSprite, int nStep );
void PutSpriteOnFloor( spritetype *pSprite, int );
void PutSpriteOnCeiling( spritetype *pSprite, int );


void ProcessKeys3D();
void processMouseLook3D(BOOL readMouse = FALSE);


char dlgSpriteText();
#endif