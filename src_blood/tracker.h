/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1997: Originally written by Nick Newhard.
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

#ifndef _TRACKER_H_
#define _TRACKER_H_
#include "db.h"

class CXTracker {
public:
    char m_type;
    short m_nSector;
    short m_nWall;
    short m_nSprite;
    sectortype* m_pSector;
    walltype* m_pWall;
    spritetype* m_pSprite;
    XSECTOR* m_pXSector;
    XWALL* m_pXWall;
    XSPRITE* m_pXSprite;
    CXTracker();
    void TrackClear();
    void TrackSector(short a1, char a2);
    void TrackWall(short a1, char a2);
    void TrackSprite(short a1, char a2);
    void Draw(int a1, int a2, int a3);
};

extern CXTracker gXTracker;

#endif
