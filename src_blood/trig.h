/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
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

#include "resource.h"

extern int costable[2048];

int GetOctant(int x, int y);
void RotateVector(int *dx, int *dy, int nAngle);
void RotatePoint(int *x, int *y, int nAngle, int ox, int oy);
void trigInit(Resource &Res);

inline int Sin(int ang)
{
    return costable[(ang - 512) & 2047];
}

inline int Cos(int ang)
{
    return costable[ang & 2047];
}