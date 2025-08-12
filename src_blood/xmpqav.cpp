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
#include "tile.h"
#include "xmpqav.h"
void DrawFrame(int x, int y, TILE_FRAME *pTile, int stat, int shade, int palnum)
{
    stat |= pTile->stat;
    int angle = pTile->angle;
    if (stat & 0x100)
    {
        angle = (angle+1024)&2047;
        stat &= ~0x100;
        stat ^= 0x4;
    }

    if (stat & kQavOrientationLeft)
        stat |= 256;

    if (!(stat & kQavOrientationQ16))
    {
        x <<= 16;
        y <<= 16;
    }

    stat &= ~(kQavOrientationLeft | kQavOrientationQ16);
    if (palnum <= 0)
        palnum = pTile->palnum;
    rotatesprite(x + (pTile->x << 16), y + (pTile->y << 16), pTile->z, angle,
                 pTile->picnum, ClipRange(pTile->shade + shade, -128, 127), palnum, stat,
                 windowx1, windowy1, windowx2, windowy2);

}

void QAV::Preload(void)
{
    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (frames[i].tiles[j].picnum >= 0)
                tilePreloadTile(frames[i].tiles[j].picnum);
        }
    }
}