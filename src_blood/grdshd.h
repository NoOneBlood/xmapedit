/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Gradient shading feature.
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

struct GRSHIGHLIGHT {
	schar  type;
	short  idx;
	short  picnum;
	schar  order;
	schar  shade;
};

#define kMaxHgltObjects 512
#define kMaxHgltParts   kMaxHgltObjects / 2
extern BOOL gResetHighlight;
extern GRSHIGHLIGHT gHglt[kMaxHgltObjects];
extern short gHgltc;

short grshHighlighted(int otype, int idx);
void grshUnhgltObjects(int hidx, BOOL erase);
BOOL grshAddObjects(schar otype, short idx);
void grshHgltObjects(int idx);
int grshShadeWalls(BOOL toggle);