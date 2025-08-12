/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of mirrors.cpp adapted for level editor's Preview Mode
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

#ifndef __XMPROR
#define __XMPROR

#define kMaxROR 16
#define kMirrorPic 504

struct MIRROR
{
    unsigned int type       : 3;
    unsigned int id         : 16;
    unsigned int basePic    : 16;
    POINT3D point;
};
extern MIRROR mirror[kMaxROR];
extern short mirrorcnt;

void InitMirrors(void);
bool DrawMirrors(int x, int y, int z, int a, int horiz);
char IsRorSector(int nSect, int stat);
bool IsLinkCorrect(spritetype* pSpr);
bool IsMirrorPic(int nPic);
void RestoreMirrorPic();
void ClearMirrorPic();
//---------------------------------------------------
void warpInit(void);
int CheckLink(spritetype *pSprite, int nID, char wallLink);
int CheckLink(int *x, int *y, int *z, int *nID, char wallLink);
int CheckLinkCamera(int *x, int *y, int *z, int *nID, char wallLink);
#endif